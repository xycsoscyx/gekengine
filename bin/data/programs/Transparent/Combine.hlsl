#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const float PixelsPerDiffusion = 200.0;
static const float PixelsPerDiffusionSquared = square(PixelsPerDiffusion);
static const int MaximumDiffusionPixels = 8;
static const int DiffusionStridePixels = 1;

// http://graphics.cs.williams.edu/papers/TransparencyI3D16/McGuire2016Transparency.pdf
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 backgroundModulationAndDiffusion = Resources::modulationDiffusionBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float3 backgroundModulation = backgroundModulationAndDiffusion.rgb;
    if (minComponent(backgroundModulation) == 1.0)
    {
        return Resources::sourceBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    }

    float diffusionSquared = backgroundModulationAndDiffusion.a * PixelsPerDiffusionSquared;
    float2 refraction = 3.0 * Resources::refractionBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0) * (1.0 / 8.0);
    float4 accumulation = Resources::accumulationBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    if (isinf(accumulation.a))
    {
        accumulation.a = maxComponent(accumulation.rgb);
    }

    if (isinf(maxComponent(accumulation.rgb)))
    {
        accumulation = (isinf(accumulation.a) ? 1.0 : accumulation.a);
    }

    accumulation.rgb *= 0.5 + backgroundModulation / max(0.01, 2.0 * maxComponent(backgroundModulation));

    float3 background = 0.0;
    if (diffusionSquared > 0.0)
    {
        static const int stride = DiffusionStridePixels;
        int radius = int(min(sqrt(diffusionSquared), MaximumDiffusionPixels) / float(stride)) * stride;

        float weightSum = 0.0;
        for (float2 sampleOffset = -radius; sampleOffset.x <= radius; sampleOffset.x += stride)
        {
            for (sampleOffset.y = -radius; sampleOffset.y <= radius; sampleOffset.y += stride)
            {
                float sampleRadius = dot(sampleOffset, sampleOffset);
                if (sampleRadius <= diffusionSquared)
                {
                    float2 sampleCoord = inputPixel.texCoord + refraction + sampleOffset * Shader::pixelSize;
                    float backgroundBlurRadiusSquared = Resources::modulationDiffusionBuffer.SampleLevel(Global::pointSampler, sampleCoord, 0).a * PixelsPerDiffusionSquared;
                    if (sampleRadius <= backgroundBlurRadiusSquared)
                    {
                        // Disc weight
                        float weight = 1.0 / backgroundBlurRadiusSquared + Math::Epsilon;

                        // Gaussian weight (slightly higher quality but much slower
                        // float weight = exp(-sampleRadius / (8 * backgroundBlurRadiusSquared)) / sqrt(4 * Math::Pi * backgroundBlurRadiusSquared);

                        background += weight * Resources::sourceBuffer.SampleLevel(Global::pointSampler, sampleCoord, 0);
                        weightSum += weight;
                    }
                }
            }
        }

        background /= weightSum;
    }
    else
    {
        background = Resources::sourceBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + refraction, 0);
    }

    return background * backgroundModulation + (1.0 - backgroundModulation) * accumulation.rgb / max(accumulation.a, Math::Epsilon);
}

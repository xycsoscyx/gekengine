#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const int maxDiffusionPixels = 16;
static const int diffusionStridePixels = 2;
static const float pixelsPerDiffusion2 = square(200.0);
static const int stride = diffusionStridePixels;

// http://graphics.cs.williams.edu/papers/TransparencyI3D16/McGuire2016Transparency.pdf
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 backgroundModulationAndDiffusion = Resources::modulationDiffusionBuffer[inputPixel.position.xy];
    float3 backgroundModulation = backgroundModulationAndDiffusion.rgb;

    [branch]
    if (minComponent(backgroundModulation) == 1.0)
    {
        return Resources::sourceBuffer[inputPixel.position.xy];
    }

    float diffusionSquared = backgroundModulationAndDiffusion.a * pixelsPerDiffusion2;
    float2 refraction = 3.0 * Resources::refractionBuffer[inputPixel.position.xy] * (1.0 / 8.0);
    float4 accumulation = Resources::accumulationBuffer[inputPixel.position.xy];

    [branch]
    if (isinf(accumulation.a))
    {
        accumulation.a = maxComponent(accumulation.rgb);
    }

    [branch]
    if (isinf(maxComponent(accumulation.rgb)))
    {
        accumulation = (isinf(accumulation.a) ? 1.0 : accumulation.a);
    }

    accumulation.rgb *= 0.5 + backgroundModulation / max(0.01, 2.0 * maxComponent(backgroundModulation));

    float3 background = 0.0;

    [branch]
    if (diffusionSquared > 0.0)
    {
        int radius = int(min(sqrt(diffusionSquared), maxDiffusionPixels) / float(stride)) * stride;

        float weightSum = 0;

        [loop]
        for (int2 tapOffset = -radius; tapOffset.x <= radius; tapOffset.x += stride)
        {
            [loop]
            for (tapOffset.y = -radius; tapOffset.y <= radius; tapOffset.y += stride)
            {
                float radiusSquared = dot(tapOffset, tapOffset);

                [branch]
                if (radiusSquared <= diffusionSquared)
                {
                    int2 tapCoord = inputPixel.position.xy + tapOffset;
                    float backgroundBlurRadiusSquared = Resources::modulationDiffusionBuffer[tapCoord].a * pixelsPerDiffusion2;

                    [branch]
                    if (radiusSquared <= backgroundBlurRadiusSquared)
                    {
                        // Disk weight
                        float weight = 1.0 / backgroundBlurRadiusSquared + Math::Epsilon;

                        // Gaussian weight (slightly higher quality but much slower
                        // float weight = exp(-radiusSquared / (8 * backgroundBlurRadiusSquared)) / sqrt(4 * pi * backgroundBlurRadiusSquared);

                        background += weight * Resources::sourceBuffer[tapCoord];
                        weightSum += weight;
                    }
                }
            }
        }

        background /= weightSum;
    }
    else
    {
        background = Resources::sourceBuffer[inputPixel.position.xy];
    }

    return background * backgroundModulation + (1.0 - backgroundModulation) * accumulation.rgb / max(accumulation.a, Math::Epsilon);
}

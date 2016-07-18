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

    float diffusion2 = backgroundModulationAndDiffusion.a * pixelsPerDiffusion2;
    float2 delta = 3.0 * Resources::refractionBuffer[inputPixel.position.xy] * (1.0 / 8.0);
    float4 accum = Resources::accumulationBuffer[inputPixel.position.xy];

    [branch]
    if (isinf(accum.a))
    {
        accum.a = maxComponent(accum.rgb);
    }

    [branch]
    if (isinf(maxComponent(accum.rgb)))
    {
        accum = (isinf(accum.a) ? 1.0 : accum.a);
    }

    accum.rgb *= 0.5 + backgroundModulation / max(0.01, 2.0 * maxComponent(backgroundModulation));

    float3 background = 0.0;

    [branch]
    if (diffusion2 > 0)
    {
        int R = int(min(sqrt(diffusion2), maxDiffusionPixels) / float(stride)) * stride;

        float weightSum = 0;

        [loop]
        for (int2 q = -R; q.x <= R; q.x += stride)
        {
            [loop]
            for (q.y = -R; q.y <= R; q.y += stride)
            {
                float radius2 = dot(q, q);

                [branch]
                if (radius2 <= diffusion2)
                {
                    int2 tap = inputPixel.position.xy + q;
                    float backgroundBlurRadius2 = Resources::modulationDiffusionBuffer[tap].a * pixelsPerDiffusion2;

                    [branch]
                    if (radius2 <= backgroundBlurRadius2)
                    {
                        // Disk weight
                        float weight = 1.0 / backgroundBlurRadius2 + 1e-5;

                        // Gaussian weight (slightly higher quality but much slower
                        // float weight = exp(-radius2 / (8 * backgroundBlurRadius2)) / sqrt(4 * pi * backgroundBlurRadius2);

                        background += weight * Resources::sourceBuffer[tap];
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

    return background * backgroundModulation + (1.0 - backgroundModulation) * accum.rgb / max(accum.a, 0.00001);
}

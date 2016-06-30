#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const float PPD = 200.0;
static const int maxDiffusionPixels = 16;

float minComponent(float3 value)
{
    return min(value.x, min(value.y, value.z));
}

float maxComponent(float3 value)
{
    return max(value.x, max(value.y, value.z));
}

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 betaDiffusion = Resources::betaBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float3 beta = betaDiffusion.rgb;
    if (minComponent(beta) == 1.0)
    {
        return Resources::backgroundBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    }

    float4 alpha = Resources::alphaBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    if (isinf(alpha.a))
    {
        alpha.a = maxComponent(alpha.rgb);
    }

    if (isinf(maxComponent(alpha.rgb)))
    {
        alpha = (isinf(alpha.a) ? 1.0 : alpha.a);
    }

    alpha.rgb *= 0.5 + 0.5 * beta / max(0.01, maxComponent(beta));

    float diffusion = betaDiffusion.a * sqr(PPD);
    if (diffusion > 0)
    {
        static const float stride = 2.0;

        float weightSum = 0.0;
        float3 background = 0.0;
        float radius = floor(min(sqrt(diffusion), maxDiffusionPixels) / float(stride)) * stride;
        for (float2 step = -radius; step.x <= radius; step.x += stride)
        {
            for (step.y = -radius; step.y <= radius; step.y += stride)
            {
                float radius2 = dot(step, step);
                if (radius2 <= diffusion)
                {
                    float2 sampleCoord = (inputPixel.texCoord + (step / Shader::targetSize));
                    float sampleDiffusion = Resources::betaBuffer.SampleLevel(Global::pointSampler, sampleCoord, 0).a;
                    float sampleRadius2 = sampleDiffusion * PPD*PPD;
                    if (radius2 <= sampleRadius2)
                    {
                        float weight = 1.0 / sampleRadius2 + 1e-5;
                        background += weight * Resources::backgroundBuffer.SampleLevel(Global::pointSampler, sampleCoord, 0);
                        weightSum += weight;
                    }
                }
            }
        }

        background /= weightSum;
        return background * beta + (1.0 - beta) * alpha.rgb / max(alpha.a, 0.00001);    }
    else
    {
        return Resources::backgroundBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    }
}

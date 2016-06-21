#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float calculateGaussianWeight(float offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * Defines::gaussianSigma));
    static const float d = rcp(2.0 * sqr(Defines::gaussianSigma));
    return (g * exp(-sqr(offset) * d)) / 2;
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize = rcp(Shader::targetSize);
    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float finalValue = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -Defines::guassianRadius; offset <= Defines::guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * Defines::blurAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float depthDelta = abs(surfaceDepth - sampleDepth);

        float sampleWeight = calculateGaussianWeight(offset) * rcp(Math::Epsilon + Defines::bilateralEdgeSharpness * depthDelta);

        float sampleValue = Resources::sourceBuffer.Sample(Global::linearClampSampler, sampleTexCoord);
        finalValue += (sampleValue * sampleWeight);
        totalWeight += sampleWeight;
    }

    return (finalValue * rcp(totalWeight + Math::Epsilon));
}

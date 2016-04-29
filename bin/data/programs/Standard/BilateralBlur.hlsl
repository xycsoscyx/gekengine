#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float calculateGaussianWeight(float offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * gaussianSigma));
    static const float d = rcp(2.0 * sqr(gaussianSigma));
    return (g * exp(-sqr(offset) * d)) / 2;
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize = rcp(Shader::targetSize);
    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float finalAmbientOcclusion = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -guassianRadius; offset <= guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * blurAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float sampleAmbientOcclusion = Resources::ambientOcclusionBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float depthDelta = abs(surfaceDepth - sampleDepth);

        float sampleWeight = calculateGaussianWeight(offset)
            * rcp(Math::Epsilon + bilateralEdgeSharpness * depthDelta);

        finalAmbientOcclusion += (sampleAmbientOcclusion * sampleWeight);
        totalWeight += sampleWeight;
    }

    return (finalAmbientOcclusion * rcp(totalWeight + Math::Epsilon));
}

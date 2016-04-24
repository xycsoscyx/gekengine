#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float calculateGaussianWeight(int offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * gaussianSigma));
    static const float d = (2.0 * sqr(gaussianSigma));
    return (g * exp(-sqr(offset) / d));
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize = rcp(Shader::targetSize);
    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float finalAmbientOcclusion = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -guassianRadius; offset < guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * blurAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float4 sampleAmbientOcclusion = Resources::ambientOcclusionBuffer.Sample(Global::linearClampSampler, sampleTexCoord);

        float sampleWeight = calculateGaussianWeight(offset);
        // range doma(the "bilateral" weight). As depth difference increases, decrease weight.
        sampleWeight *= rcp(Math::Epsilon + bilateralEdgeSharpness * abs(surfaceDepth - sampleDepth));

        finalAmbientOcclusion += (sampleAmbientOcclusion * sampleWeight);
        totalWeight += sampleWeight;
    }

    finalAmbientOcclusion *= rcp(totalWeight + Math::Epsilon);
    return finalAmbientOcclusion;
}

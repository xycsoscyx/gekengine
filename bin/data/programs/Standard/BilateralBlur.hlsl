#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float calculateGaussianWeight(int offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * gaussianSigma));
    static const float d = (2.0 * sqr(gaussianSigma));
    return (g * exp(-sqr(offset) / d));
}

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize;
    Resources::effectsBuffer.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize = rcp(pixelSize);

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float4 finalEffects = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -guassianRadius; offset < guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * blurAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float4 sampleEffects = Resources::effectsBuffer.Sample(Global::linearClampSampler, sampleTexCoord);

        float4 sampleWeight = calculateGaussianWeight(offset);
        // range doma(the "bilateral" weight). As depth difference increases, decrease weight.
        sampleWeight.w *= rcp(Math::Epsilon + bilateralEdgeSharpness * abs(surfaceDepth - sampleDepth));

        finalEffects += (sampleEffects * sampleWeight);
        totalWeight += sampleWeight.w;
    }

    finalEffects.w *= rcp(totalWeight + Math::Epsilon);
    return finalEffects;
}

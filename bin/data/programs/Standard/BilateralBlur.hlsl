#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float calculateGaussianWeight(int offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * gaussianSigma));
    static const float d = (2.0 * sqr(gaussianSigma));
    return (g * exp(-sqr(offset) / d));
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize;
    Resources::ambientObscuranceBuffer.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize = rcp(pixelSize);

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float finalOcclusion = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -guassianRadius; offset < guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * blurAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float sampleOcclusion = Resources::ambientObscuranceBuffer.Sample(Global::linearSampler, sampleTexCoord);

        float sampleWeight = calculateGaussianWeight(offset);
        // range doma(the "bilateral" weight). As depth difference increases, decrease weight.
        sampleWeight *= rcp(Math::Epsilon + bilateralEdgeSharpness * abs(surfaceDepth - sampleDepth));

        finalOcclusion += (sampleOcclusion * sampleWeight);
        totalWeight += sampleWeight;
    }

    return (finalOcclusion * rcp(totalWeight + Math::Epsilon));
}

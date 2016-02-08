#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float calculateGaussianWeight(int offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * blurSigma));
    static const float d = (2.0 * sqr(blurSigma));
    return (g * exp(-sqr(offset) / d));
}

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize;
    Resources::effectsBuffer.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize = rcp(pixelSize);

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float4 finalColor = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -guassianRadius; offset < guassianRadius; offset++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * bloomAxis));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float4 sampleColor = Resources::effectsBuffer.Sample(Global::linearSampler, sampleTexCoord);

        float4 sampleWeight = calculateGaussianWeight(offset);

        // range doma(the "bilateral" weight). As depth difference increases, decrease weight.
        sampleWeight.w *= rcp(Math::Epsilon + bilateralEdgeSharpness * abs(surfaceDepth - sampleDepth));

        finalColor += (sampleColor * sampleWeight);
        totalWeight += sampleWeight.w;
    }

    finalColor.w *= rcp(totalWeight + Math::Epsilon);
    return finalColor;
}

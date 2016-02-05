#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float CalculateGaussianWeight(int offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * bloomSigma));
    static const float d = (2.0 * sqr(bloomSigma));
    return (g * exp(-sqr(offset) / d));
}

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float2 pixelSize;
    Resources::effectsBuffer.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize = rcp(pixelSize);

    float4 finalColor = 0.0;

    [unroll]
    for (int offset = -bloomRadius; offset < bloomRadius; offset++)
    {
        float sampleWeight = CalculateGaussianWeight(offset);
        float2 sampleTexCoord = (inputPixel.texCoord + (offset * pixelSize * bloomAxis));
        float4 sampleColor = Resources::effectsBuffer.Sample(Global::linearSampler, sampleTexCoord);
        finalColor += (sampleColor * sampleWeight);
    }

    return finalColor;
}

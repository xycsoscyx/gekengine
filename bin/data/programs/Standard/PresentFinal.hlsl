#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float3 getToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}

static const float2 bilateralSamples[5] = 
{
    float2( 0.0, 0.0),
    float2(-1.0, 0.0),
    float2( 1.0, 0.0),
    float2( 0.0,-1.0),
    float2( 0.0, 1.0),
};

float getFinalShadowFactor(InputPixel inputPixel)
{
    float2 pixelSize;
    Resources::effectsBuffer.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize = rcp(pixelSize);

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float totalShadow = 0.0;
    float totalWeight = 0.0;
    for (uint index = 0; index < 5; index++)
    {
        float2 sampleTexCoord = (inputPixel.texCoord + (bilateralSamples[index] * pixelSize));
        float sampleDepth = Resources::depthBuffer.Sample(Global::pointSampler, sampleTexCoord);
        float sampleShadow = Resources::effectsBuffer.Sample(Global::linearClampSampler, sampleTexCoord).w;

        // range doma(the "bilateral" weight). As depth difference increases, decrease weight.
        float weight = rcp(Math::Epsilon + bilateralEdgeSharpness * abs(surfaceDepth - sampleDepth));

        totalShadow += (sampleShadow * weight);
        totalWeight += weight;
    }

    totalShadow *= rcp(totalWeight + Math::Epsilon);
    return totalShadow;
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));
    float3 baseColor = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float4 specialEffects = Resources::effectsBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return getFinalShadowFactor(inputPixel);
    baseColor *= specialEffects.w;

    float exposure = 0.0;
    float3 exposedColor = getExposedColor(baseColor, averageLuminance, exposure);
    float3 finalColor = getToneMapFilmicALU(exposedColor + specialEffects.xyz);

    return finalColor;
}
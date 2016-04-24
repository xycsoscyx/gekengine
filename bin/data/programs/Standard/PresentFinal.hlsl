#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 getToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));
    float3 baseColor = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return baseColor;
    float ambientOcclusion = Resources::ambientOcclusionBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    //baseColor *= ambientOcclusion;

    float exposure = 0.0;
    float3 exposedColor = getExposedColor(baseColor, averageLuminance, exposure);
    float3 finalColor = getToneMapFilmicALU(exposedColor);

    return finalColor;
}
#include "GEKEngine"

#include "GEKGlobal.h"

float3 CalculateExposedColor(float3 color, float avgLuminance, float threshold, out float exposure)
{
    float KeyValue = 0.2f;

    // Use geometric mean
    avgLuminance = max(avgLuminance, 0.001f);
    float keyValue = KeyValue;
    float linearExposure = (KeyValue / avgLuminance);
    exposure = log2(max(linearExposure, Math::Epsilon));
    exposure -= threshold;
    return exp2(exposure) * color;
}

float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float threshold = 0.0;
    float exposure = 0.0;

    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));
    float4 effectsColor = Resources::effectsBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return effectsColor.w;
    float3 baseColor = (Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord) * effectsColor.w);
    float3 exposedColor = CalculateExposedColor(baseColor, averageLuminance, threshold, exposure);
    float3 finalColor = ToneMapFilmicALU(exposedColor);

    return (finalColor + (effectsColor.xyz * bloomScale));
}
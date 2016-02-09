#include "GEKEngine"

#include "GEKGlobal.h"

float3 calculateExposedColor(float3 pixelColor, float averageLuminance, float threshold, out float exposure)
{
    float KeyValue = 0.2f;

    // Use geometric mean
    averageLuminance = max(averageLuminance, 0.001f);
    float keyValue = KeyValue;
    float linearExposure = (KeyValue / averageLuminance);
    exposure = log2(max(linearExposure, Math::Epsilon));
    exposure -= threshold;
    return exp2(exposure) * pixelColor;
}

float3 toneMapFilmicALU(float3 pixelColor)
{
    pixelColor = max(0, pixelColor - 0.004f);
    pixelColor = (pixelColor * (6.2f * pixelColor + 0.5f)) / (pixelColor * (6.2f * pixelColor + 1.7f) + 0.06f);
    // result has 1/2.2 baked in
    return pow(pixelColor, 2.2f);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));
    float4 effectsColor = Resources::effectsBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return effectsColor.w;
    float3 baseColor = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float exposure = 0.0;
    float3 exposedColor = calculateExposedColor(baseColor, averageLuminance, toneMappingThreshold, exposure);
    float3 finalColor = toneMapFilmicALU((exposedColor * effectsColor.w) + effectsColor.xyz);

    return finalColor;
}
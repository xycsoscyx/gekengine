#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float CalculateLuminance(float3 color)
{
    return max(dot(color, float3(0.299, 0.587, 0.114)), 0.0001);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 color = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float currentLuminance = CalculateLuminance(color);
    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));
    float3 delta = (color * (currentLuminance / averageLuminance));
    return max(0.0f, (delta - color));
}

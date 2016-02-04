#include "GEKEngine"

#include "GEKGlobal.h"

float CalculateLuminance(float3 color)
{
    return max(dot(color, float3(0.299, 0.587, 0.114)), 0.0001);
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 color = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return log2(max(CalculateLuminance(color), 0.00001f));
}

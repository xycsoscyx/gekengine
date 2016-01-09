#include "GEKEngine"

#include "GEKGlobal.h"

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return max(dot(color, float3(0.299, 0.587, 0.114)), 0.0001);
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 color = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord).rgb;
    return log2(1.0e-5 + CalcLuminance(color));
}

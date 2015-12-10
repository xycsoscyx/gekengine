#include "GEKEngine"

#include "GEKGlobal.h"

// Approximates luminance from an RGB value
float CalcLuminance(float3 color)
{
    return max(dot(color, float3(0.299f, 0.587f, 0.114f)), 0.0001f);
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 color = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord).rgb;
    return log2(1e-5 + CalcLuminance(color));
}

#include "GEKEngine"

#include "GEKGlobal.h"

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float3 lightingColor = Resources::lightingBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    return float4(lightingColor, 0.0f);
}
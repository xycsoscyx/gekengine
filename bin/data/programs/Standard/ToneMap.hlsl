#include "GEKEngine"

#include "GEKGlobal.h"

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float exposure = Resources::compositeBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 5).a;
    float3 color = Resources::compositeBuffer.Sample(Global::pointSampler, inputPixel.texCoord).rgb;
    return float4(color, 1.0f);
}
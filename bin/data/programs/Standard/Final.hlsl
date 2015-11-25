#include "GEKEngine"

#include "GEKGlobal.h"

static const float crush = 0.05;
static const float frange = 10;
static const float exposure = 10;

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float exposure = Resources::exposureBuffer[0];
    float3 color = Resources::compositeBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return float4(smoothstep(crush, frange + crush, log2(1 + color * exposure)), 1.0f);
}
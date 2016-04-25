#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
}

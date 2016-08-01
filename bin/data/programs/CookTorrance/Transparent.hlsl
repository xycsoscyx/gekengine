#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    return float4((albedo.rgb * albedo.a), albedo.a);
}

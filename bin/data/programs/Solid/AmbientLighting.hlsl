#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float3 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    const float3 albedo = Resources::albedoBuffer[inputPixel.screen.xy];
    const float ambient = Resources::ambientBuffer[inputPixel.screen.xy];
    return (albedo * ambient * 0.01);
}
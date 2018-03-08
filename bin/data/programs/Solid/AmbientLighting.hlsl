#include <GEKEngine>

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float ambient = Resources::ambientBuffer[inputPixel.screen.xy];
    float3 albedo = Resources::albedoBuffer[inputPixel.screen.xy];
    return (ambient * albedo) * 0.025;
}

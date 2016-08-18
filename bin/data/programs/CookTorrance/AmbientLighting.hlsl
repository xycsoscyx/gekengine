#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 albedo = Resources::albedoBuffer[inputPixel.screen.xy];
    float ambient = Resources::ambientBuffer[inputPixel.screen.xy];
    ambient = ((ambient * 0.001) + 0.001);
    return (albedo * ambient);
}
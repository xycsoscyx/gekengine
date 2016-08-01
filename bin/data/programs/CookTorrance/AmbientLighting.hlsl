#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 albedo = Resources::albedoBuffer[inputPixel.position.xy];
    float ambient = ((Resources::ambientBuffer[inputPixel.position.xy] * 0.007) + 0.002);
    return (albedo * ambient);
}
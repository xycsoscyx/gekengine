#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const uint2 screenCoord = uint2(inputPixel.screen.xy);
    float ambient = Resources::ambientBuffer[screenCoord];
    float3 albedo = Resources::albedoBuffer[screenCoord];
    return (ambient * albedo) * 0.025;
}

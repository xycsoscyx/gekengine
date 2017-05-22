#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float ambient = Resources::ambientBuffer.SampleLevel(Global::PointSampler, inputPixel.texCoord, 0);
    float3 albedo = Resources::albedoBuffer.SampleLevel(Global::PointSampler, inputPixel.texCoord, 0);
    return (ambient * albedo) * 0.025;
}

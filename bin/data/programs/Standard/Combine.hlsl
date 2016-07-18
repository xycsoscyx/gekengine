#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float ambientOcclusion = Resources::ambientOcclusionBuffer[inputPixel.position.xy];
    float3 baseColor = Resources::lightAccumulationBuffer[inputPixel.position.xy];
    return (baseColor * ambientOcclusion);
}
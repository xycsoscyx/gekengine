#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float ambientOcclusion = Resources::ambientOcclusionBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 baseColor = Resources::lightAccumulationBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    return (baseColor * ambientOcclusion);
}
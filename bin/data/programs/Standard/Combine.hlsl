#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float ambientOcclusion = Resources::ambientOcclusionBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float3 baseColor = Resources::lightAccumulationBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    return (baseColor * ambientOcclusion);
}
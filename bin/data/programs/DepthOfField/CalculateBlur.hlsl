#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float focalDepth = Resources::focalDepth[0];
    float surfaceDepth = Resources::depth[inputPixel.screen.xy];
    return (surfaceDepth > focalDepth ? float3(1, 0, 0) : float3(0, 0, 1));
}
#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 backColor = Resources::backBuffer[inputPixel.screen.xy];
    float3 finalColor = Resources::finalBuffer[inputPixel.screen.xy];
    float4 frontColor = Resources::frontBuffer[inputPixel.screen.xy];
    return backColor.rgb + frontColor.rgb;
}
#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float averageDepth = Resources::averageFocalDepth[0];
    float surfaceDepth = getLinearDepthFromSample(Resources::depthBuffer[inputPixel.screen.xy]);
    float3 finalColor = Resources::finalBuffer[inputPixel.screen.xy];

    float backInner = (averageDepth + Defines::focusInnerDepth);
    float backOuter = (backInner + Defines::focusOuterDepth);
    float backFactor = saturate((surfaceDepth - backInner) / backOuter);

    float frontInner = (averageDepth - Defines::focusInnerDepth);
    float frontOuter = (frontInner - Defines::focusOuterDepth);
    float frontFactor = saturate((frontInner - surfaceDepth) / frontOuter);

    OutputPixel outputPixel;
    outputPixel.backBuffer = (float4(finalColor, 1.0) * backFactor);
    outputPixel.frontBuffer = (float4(finalColor, 1.0) * frontFactor);
    return outputPixel;
}
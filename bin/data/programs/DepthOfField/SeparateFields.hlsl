#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float focalDepth = Resources::averageFocalDepth[0];
    float sceneDepth = getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0));
    float foregroundDistance = saturate((focalDepth - sceneDepth) / Defines::focalRange);

    float3 color = Resources::screen.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);

    OutputPixel outputPixel;
    outputPixel.background = color;
    outputPixel.foreground = float4((color * foregroundDistance), foregroundDistance);
    return outputPixel;
}
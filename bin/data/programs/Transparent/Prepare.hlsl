#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float sceneDepth = Resources::depthBuffer[inputPixel.position.xy];
    float depth = (inputPixel.viewPosition.z / Camera::maximumDistance);
    float depthDelta = saturate((sceneDepth - depth) * Camera::maximumDistance);

    float4 color = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    color.a *= depthDelta;

    float reveal = max(min(1.0, max(max(color.r, color.g), color.b) * color.a), color.a)
        * clamp(0.03 / (1e-5 + pow(-depth, 4.0)), 1e-2, 3e3); // Eq 9

    OutputPixel outputPixel;
    outputPixel.accumulationBuffer = (float4((color.rgb * color.a), color.a) * reveal);
    outputPixel.revealBuffer = color.a;
    return outputPixel;
}
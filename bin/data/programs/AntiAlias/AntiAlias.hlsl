#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

namespace Defines
{
    static const float depthBias = 0.004;
    static const float depthThreshold = 0.3;
};

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float centerDepth = Resources::depthBuffer[inputPixel.screen.xy];
    float sampleDepthP0 = Resources::depthBuffer[inputPixel.screen.xy + float2(+1.0, +0.0)];
    float sampleDepthN0 = Resources::depthBuffer[inputPixel.screen.xy + float2(-1.0, +0.0)];
    float sampleDepth0P = Resources::depthBuffer[inputPixel.screen.xy + float2(+0.0, +1.0)];
    float sampleDepth0N = Resources::depthBuffer[inputPixel.screen.xy + float2(+0.0, -1.0)];
    float sampleDepthNN = Resources::depthBuffer[inputPixel.screen.xy + float2(-1.0, -1.0)];
    float sampleDepthPP = Resources::depthBuffer[inputPixel.screen.xy + float2(+1.0, +1.0)];
    float sampleDepthNP = Resources::depthBuffer[inputPixel.screen.xy + float2(-1.0, +1.0)];
    float sampleDepthPN = Resources::depthBuffer[inputPixel.screen.xy + float2(+1.0, -1.0)];

    float4 edgeDepth;
    edgeDepth.x = (sampleDepthP0 + sampleDepthN0);
    edgeDepth.y = (sampleDepth0P + sampleDepth0N);
    edgeDepth.z = (sampleDepthNN + sampleDepthPP);
    edgeDepth.w = (sampleDepthNP + sampleDepthPN);
    edgeDepth = abs((2.0 * centerDepth) - edgeDepth) - Defines::depthBias;
    edgeDepth = step(edgeDepth, 0.0);
    return (1.0 - saturate(dot(edgeDepth, Defines::depthThreshold)));
}
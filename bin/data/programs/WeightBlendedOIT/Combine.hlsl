#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 accumulation = Resources::accumulationBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float reveal = Resources::revealBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float3 average = (accumulation.rgb / max(accumulation.a, Math::Epsilon));
    return float4(average, reveal);
}

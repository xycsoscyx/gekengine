#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float4 blend = Resources::blendBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float weight = Resources::weightBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    float3 average = blend.rgb / max(blend.a, 0.00001);
    return float4(average * (1 - weight), weight);

}

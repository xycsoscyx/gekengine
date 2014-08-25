SamplerState  gs_pPointSampler			: register(s0);

Texture2D     gs_pTextureBuffer         : register(t0);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
    float4 color                        : COLOR0;
}; 

float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    return (kInput.color * gs_pTextureBuffer.Sample(gs_pPointSampler, kInput.texcoord));
}
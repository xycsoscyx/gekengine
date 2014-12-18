cbuffer ENGINEBUFFER                    : register(b0)
{
    float2   gs_nCameraFieldOfView      : packoffset(c0);
    float    gs_nCameraMinDistance      : packoffset(c0.z);
    float    gs_nCameraMaxDistance      : packoffset(c0.w);
    float4x4 gs_nViewMatrix             : packoffset(c1);
    float4x4 gs_nProjectionMatrix       : packoffset(c5);
    float4x4 gs_nInvProjectionMatrix    : packoffset(c9);
    float4x4 gs_nTransformMatrix        : packoffset(c13);
};

SamplerState  gs_pPointSampler			: register(s0);

Texture2D     gs_pScreenBuffer          : register(t0);
Texture2D     gs_pOverlayBuffer         : register(t1);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
}; 

float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    float4 nScreen = gs_pScreenBuffer.Sample(gs_pPointSampler, kInput.texcoord);
    float4 nOverlay = gs_pOverlayBuffer.Sample(gs_pPointSampler, kInput.texcoord);
    return ((nScreen * (1.0f - nOverlay.a)) + (nOverlay * nOverlay.a));
}
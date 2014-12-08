cbuffer ENGINEBUFFER                    : register(b0)
{
    float2   gs_nCameraFieldOfView      : packoffset(c0);
    float    gs_nCameraMinDistance      : packoffset(c0.z);
    float    gs_nCameraMaxDistance      : packoffset(c0.w);
    float3   gs_nCameraPosition         : packoffset(c1);
    float4x4 gs_nViewMatrix             : packoffset(c2);
    float4x4 gs_nProjectionMatrix       : packoffset(c6);
    float4x4 gs_nInvProjectionMatrix    : packoffset(c10);
    float4x4 gs_nTransformMatrix        : packoffset(c14);
};

SamplerState  gs_pPointSampler			: register(s0);
SamplerState  gs_pLinearSampler			: register(s1);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
    float3 view                         : TEXCOORD1;
};

_INSERT_PIXEL_PROGRAM

cbuffer ENGINEBUFFER                    : register(b0)
{
    float2   gs_nCameraFieldOfView      : packoffset(c0);
    float    gs_nCameraMinDistance      : packoffset(c0.z);
    float    gs_nCameraMaxDistance      : packoffset(c0.w);
    float3   gs_nCameraPosition         : packoffset(c1);
    float4x4 gs_nViewMatrix             : packoffset(c2);
    float4x4 gs_nProjectionMatrix       : packoffset(c6);
    float4x4 gs_nTransformMatrix        : packoffset(c10);
};

cbuffer MATERIALBUFFER                  : register(b1)
{
    float4  gs_nMaterialColor           : packoffset(c0);
    bool    gs_bMaterialFullBright      : packoffset(c1);
    float3  gs_nMaterialPadding         : packoffset(c1.y);
};

SamplerState  gs_pPointSampler			: register(s0);
SamplerState  gs_pLinearSampler			: register(s1);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float4 viewposition                 : TEXCOORD0;
    float2 texcoord                     : TEXCOORD1;
    float3 viewnormal                   : NORMAL0;
    float4 color                        : COLOR0;
    bool   frontface                    : SV_ISFRONTFACE;
};

_INSERT_PIXEL_PROGRAM

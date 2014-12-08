SamplerState gs_pPointSampler			: register(s0);
SamplerState gs_pLinearSampler			: register(s1);

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

cbuffer MATERIALBUFFER                  : register(b1)
{
    float4  gs_nMaterialColor           : packoffset(c0);
    bool    gs_bMaterialFullBright      : packoffset(c1);
    float3  gs_nMaterialPadding         : packoffset(c1.y);
};

cbuffer LIGHTBUFFER                     : register(b2)
{
    uint    gs_nNumLights               : packoffset(c0);
    uint3   gs_nLightPadding            : packoffset(c0.y);
};

struct LIGHT
{
    float3  m_nPosition;
    float   m_nRange;
    float3  m_nColor;
    float   m_nInvRange;
};

StructuredBuffer<LIGHT> gs_aLights      : register(t0);

struct INPUT
{
    float4  position                     : SV_POSITION;
    float2  texcoord                     : TEXCOORD0;
    float3  view                         : TEXCOORD1;
};

_INSERT_PIXEL_PROGRAM

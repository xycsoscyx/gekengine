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

struct LIGHT
{
    float3 m_nPosition;
    float  m_nRange;
    float3 m_nColor;
    float  m_nInvRange;
};

StructuredBuffer<LIGHT> gs_aLights      : register(t0);

cbuffer LIGHTBUFFER                     : register(b1)
{
    uint    gs_nNumLights               : packoffset(c0);
    uint3   gs_nLightPadding            : packoffset(c0.y);
};

_INSERT_COMPUTE_PROGRAM

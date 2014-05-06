SamplerState  gs_pPointSampler			: register(s0);
SamplerState  gs_pLinearSampler			: register(s1);

cbuffer ENGINEBUFFER                    : register(b0)
{
    float4x4 gs_nViewMatrix;
    float4x4 gs_nProjectionMatrix;
    float4x4 gs_nTransformMatrix;
    float3   gs_nCameraPosition;
    float    gs_nCameraViewDistance;
    float2   gs_nCameraView;
    float2   gs_nCameraSize;
};

struct LIGHT
{
    float3 m_nPosition;
    float  m_nRange;
    float3 m_nColor;
    float  m_nInvRange;
};

StructuredBuffer<LIGHT> gs_aLights      : register(t0);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
    float3 view                         : TEXCOORD1;
};

_INSERT_PIXEL_PROGRAM

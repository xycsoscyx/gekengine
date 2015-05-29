static const float gs_nPI               = 3.14159265358979323846f;
static const float gs_nReciprocalPI     = rcp(gs_nPI);
static const float gs_nTwoPi            = 2.0f * gs_nPI;

SamplerState  gs_pPointSampler          : register(s0);
SamplerState  gs_pLinearSampler         : register(s1);

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


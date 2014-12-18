cbuffer ENGINEBUFFER                    : register(b0)
{
    float2   gs_nCameraFieldOfView      : packoffset(c0);
    float    gs_nCameraMinDistance      : packoffset(c0.z);
    float    gs_nCameraMaxDistance      : packoffset(c0.w);
    float2   gs_nViewPortPosition       : packoffset(c1);
    float2   gs_nViewPortSize           : packoffset(c1.z);
    float4x4 gs_nViewMatrix             : packoffset(c2);
    float4x4 gs_nProjectionMatrix       : packoffset(c6);
    float4x4 gs_nInvProjectionMatrix    : packoffset(c10);
    float4x4 gs_nTransformMatrix        : packoffset(c14);
};

_INSERT_COMPUTE_PROGRAM

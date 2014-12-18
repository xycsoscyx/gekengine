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

cbuffer ORTHOBUFFER                     : register(b1)
{
    float4x4 gs_nOrthoMatrix;
};

struct VERTEX
{
    float2 position                     : POSITION;
};

struct PIXEL
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
};

PIXEL MainVertexProgram(in VERTEX kVertex)
{
    PIXEL kPixel;
    kPixel.position = mul(gs_nOrthoMatrix, float4(kVertex.position, 0.0f, 1.0f));
    kPixel.texcoord = kVertex.position.xy;
    return kPixel;
}

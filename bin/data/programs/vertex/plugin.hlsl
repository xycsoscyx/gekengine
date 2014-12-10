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

struct WORLDVERTEX
{
    float4   position                   : POSITION;
    float4   color                      : COLOR0;
    float2   texcoord                   : TEXCOORD0;
    float3   normal                     : TEXCOORD1;
};

struct VIEWVERTEX
{
    float4   position                   : SV_POSITION;
    float2   depth                      : TEXCOORD0;
    float2   texcoord                   : TEXCOORD1;
    float3   viewnormal                 : NORMAL0;
    float4   color                      : COLOR0;
};

_INSERT_WORLD_PROGRAM

VIEWVERTEX MainVertexProgram(in SOURCEVERTEX kSource)
{
    WORLDVERTEX kVertex = GetWorldVertex(kSource);

    VIEWVERTEX kOutput;
    kOutput.position        = mul(gs_nTransformMatrix, kVertex.position);
    kOutput.depth           = kOutput.position.zw;
    kOutput.texcoord        = kVertex.texcoord;
    kOutput.viewnormal      = mul((float3x3)gs_nViewMatrix, kVertex.normal);
    kOutput.color           = kVertex.color;
    return kOutput;
}

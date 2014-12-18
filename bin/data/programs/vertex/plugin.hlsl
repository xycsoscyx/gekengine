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
    float4   viewposition               : TEXCOORD0;
    float2   texcoord                   : TEXCOORD1;
    float3   viewnormal                 : NORMAL0;
    float4   color                      : COLOR0;
};

_INSERT_WORLD_PROGRAM

VIEWVERTEX MainVertexProgram(in SOURCEVERTEX kSource)
{
    WORLDVERTEX kVertex = GetWorldVertex(kSource);

    VIEWVERTEX kOutput;
    kOutput.viewposition    = mul(gs_nViewMatrix, kVertex.position);
    kOutput.position        = mul(gs_nProjectionMatrix, kOutput.viewposition);
    kOutput.texcoord        = kVertex.texcoord;
    kOutput.viewnormal      = mul((float3x3)gs_nViewMatrix, kVertex.normal);
    kOutput.color           = kVertex.color;
    return kOutput;
}

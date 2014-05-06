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

struct VERTEX
{
    float4   position                   : POSITION;
    float2   texcoord                   : TEXCOORD0;
    float3x3 texbasis                   : TEXCOORD1;
};

struct OUTPUT
{
    float4   position                   : SV_POSITION;
    float2   texcoord                   : TEXCOORD0;
    float4   worldposition              : TEXCOORD1;
    float3x3 texbasis                   : TEXCOORD2;
};

_INSERT_WORLD_PROGRAM

OUTPUT MainVertexProgram(in SOURCEVERTEX kSource)
{
    VERTEX kVertex = SourceVertexProgram(kSource);

    OUTPUT kOutput;
    kOutput.position      = mul(gs_nTransformMatrix, kVertex.position);
    kOutput.texcoord      = kVertex.texcoord;
    kOutput.worldposition = kVertex.position;
    kOutput.texbasis      = kVertex.texbasis;
    return kOutput;
}

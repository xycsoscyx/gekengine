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

WORLDVERTEX GetWorldVertex(in SOURCEVERTEX kSource);

VIEWVERTEX MainVertexProgram(in SOURCEVERTEX kSource)
{
    WORLDVERTEX kVertex = GetWorldVertex(kSource);

    VIEWVERTEX kOutput;
    kOutput.viewposition = mul(gs_nViewMatrix, kVertex.position);
    kOutput.position = mul(gs_nProjectionMatrix, kOutput.viewposition);
    kOutput.texcoord = kVertex.texcoord;
    kOutput.viewnormal = mul((float3x3)gs_nViewMatrix, kVertex.normal);
    kOutput.color = kVertex.color;
    return kOutput;
}

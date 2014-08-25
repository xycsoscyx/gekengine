cbuffer ORTHOBUFFER                     : register(b0)
{
    float4x4 gs_nOrthoMatrix;
};

struct VERTEX
{
    float2 position                     : POSITION;
    float2 texcoord 					: TEXCOORD0;
    float4 color                        : COLOR0;
};

struct PIXEL
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
    float4 color                        : COLOR0;
};

PIXEL MainVertexProgram(in VERTEX kVertex)
{
    PIXEL kPixel;
    kPixel.position = mul(gs_nOrthoMatrix, float4(kVertex.position, 0.0f, 1.0f));
    kPixel.texcoord = kVertex.texcoord;
    kPixel.color    = kVertex.color;
    return kPixel;
}

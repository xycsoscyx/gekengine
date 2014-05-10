cbuffer ENGINEBUFFER                    : register(b0)
{
    float2   gs_nCameraSize             : packoffset(c0);
    float2   gs_nCameraView             : packoffset(c0.z);
    float    gs_nCameraViewDistance     : packoffset(c1);
    float3   gs_nCameraPosition         : packoffset(c1.y);
    float4x4 gs_nViewMatrix             : packoffset(c2);
    float4x4 gs_nProjectionMatrix       : packoffset(c6);
    float4x4 gs_nTransformMatrix        : packoffset(c10);
};

cbuffer ORTHOBUFFER                     : register(b1)
{
    float4x4 gs_nOrthoMatrix;
};

struct VERTEX
{
    float2 position                     : POSITION;
	float2 view							: TEXCOORD0;
};

struct PIXEL
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
	float3 view							: TEXCOORD1;
};

PIXEL MainVertexProgram(in VERTEX kVertex)
{
    PIXEL kPixel;
    kPixel.position = mul(gs_nOrthoMatrix, float4(kVertex.position, 0.0f, 1.0f));
    kPixel.texcoord = kVertex.position.xy;
	kPixel.view.xy = (kVertex.view * gs_nCameraView);
	kPixel.view.z = 1.0f;
    return kPixel;
}

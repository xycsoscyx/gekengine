#include "..\gekengine.h"

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

Texture2D     gs_pBuffer                : register(t0);

float4 MainPixelProgram(PIXEL kInput) : SV_TARGET
{
    return gs_pBuffer.Sample(gs_pPointSampler, kInput.texcoord);
}


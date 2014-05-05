SamplerState  gs_pPointSampler			: register(s0);
SamplerState  gs_pLinearSampler			: register(s1);

cbuffer ENGINEBUFFER                    : register(b0)
{
    float4x4 gs_nViewMatrix;
    float4x4 gs_nProjectionMatrix;
    float4x4 gs_nTransformMatrix;
    float3   gs_nCameraPosition;
	float    gs_nCameraViewDistance;
	float2   gs_nCameraView;
	float2   gs_nPadding;
};

struct INPUT
{
    float4   position                   : SV_POSITION;
    float2   texcoord                   : TEXCOORD0;
    float4   worldposition              : TEXCOORD1;
    float3x3 texbasis                   : TEXCOORD2;
    bool     frontface                  : SV_ISFRONTFACE;
};

_INSERT_PIXEL_PROGRAM

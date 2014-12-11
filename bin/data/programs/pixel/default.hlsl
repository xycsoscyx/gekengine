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

SamplerState  gs_pPointSampler			: register(s0);
SamplerState  gs_pLinearSampler			: register(s1);

struct INPUT
{
    float4 position                     : SV_POSITION;
    float2 texcoord                     : TEXCOORD0;
};

float3x3 GetCoTangentFrame(float3 nPosition, float3 nNormal, float2 nTexCoord)
{
    // get edge vectors of the pixel triangle
    float3 nPositionDX = ddx(nPosition);
    float3 nPositionDY = ddy(nPosition);
    float2 nTexCoordDX = ddx(nTexCoord);
    float2 nTexCoordDY = ddy(nTexCoord);

    // solve the linear system
    float3 nPerpendicularDX = cross(nNormal, nPositionDX);
    float3 nPerpendicularDY = cross(nPositionDY, nNormal);
    float3 nTangent = nPerpendicularDY * nTexCoordDX.x + nPerpendicularDX * nTexCoordDY.x;
    float3 nBiTangent = nPerpendicularDY * nTexCoordDX.y + nPerpendicularDX * nTexCoordDY.y;

    // construct a scale-invariant frame 
    float nReciprocal = rsqrt(max(dot(nTangent, nTangent), dot(nBiTangent, nBiTangent)));
    return float3x3(nTangent * nReciprocal, nBiTangent * nReciprocal, nNormal);
}

half2 EncodeNormal(half3 nNormal)
{
    half2 nEncoded = (normalize(nNormal.xy) * (sqrt((-nNormal.z * 0.5) + 0.5)));
    nEncoded = ((nEncoded * 0.5) + 0.5);
    return nEncoded;
}

half3 DecodeNormal(half2 nEncoded)
{
    half4 nNormal = half4((nEncoded * 2 - 1), 1, -1);
    half nLength = dot(nNormal.xyz, -nNormal.xyw);
    nNormal.z = nLength;
    nNormal.xy *= sqrt(nLength);
    return ((nNormal.xyz * 2.0f) + half3(0, 0, -1));
}

float3 GetViewPosition(float2 nTexCoord, float nDepth)
{
    nTexCoord.y = 1 - nTexCoord.y;
    nTexCoord = (nTexCoord * 2 - 1);
    float3 nViewVector = float3((nTexCoord * gs_nCameraFieldOfView), 1.0);
    return (nViewVector * nDepth * gs_nCameraMaxDistance);
}

_INSERT_PIXEL_PROGRAM

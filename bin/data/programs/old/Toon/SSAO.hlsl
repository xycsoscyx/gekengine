#include "GEKGlobal.h"
#include "GEKtypes.h"
#include "GEKutility.h"

Texture2D           gs_pAlbedoBuffer        : register(t1);
Texture2D<half2>    gs_pNormalBuffer        : register(t2);
Texture2D<float>    gs_pDepthBuffer         : register(t3);

float2 TapLocation(float nSample, float nRandomAngle)
{
	float nAlpha = (nSample + 0.5) * (1.0 / gs_nNumSamples);
	float nAngle = nAlpha * (gs_nNumSpirals * gs_nTwoPi) + nRandomAngle;
	return nAlpha * float2(cos(nAngle), sin(nAngle));
}
            
float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    float nCenterDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, kInput.texCoord);
    float3 nCenterPosition = GetViewPosition(kInput.texCoord, nCenterDepth);
	float3 nCenterNormal = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, kInput.texCoord));

	int2 nPixelCoord = kInput.position.xy;
	float nRandomAngle = (3 * nPixelCoord.x ^ nPixelCoord.y + nPixelCoord.x * nPixelCoord.y) * 10;
    float nSampleRadius = gs_nRadius / (2 * (nCenterDepth * Camera::maximumDistance) * Camera::fieldOfView.x);

    float nAmbientOcclusion = 0;

	[unroll]
	for(int nSample = 0; nSample < gs_nNumSamples; nSample++)
	{
	    float2 nSampleOffset = TapLocation(nSample, nRandomAngle);
        float2 nSampleCoord = kInput.texCoord + (nSampleOffset * nSampleRadius);
        float nSampleDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, nSampleCoord);
        float3 nSamplePosition = GetViewPosition(nSampleCoord, nSampleDepth);

		float3 nDelta = nSamplePosition - nCenterPosition;
		float nDeltaDelta = dot(nDelta, nDelta);
		float nDeltaAngle = dot(nDelta, nCenterNormal);
        nAmbientOcclusion += max(0, nDeltaAngle + nCenterDepth * 0.001) / (nDeltaDelta + gs_nEpsilon);
	}

	nAmbientOcclusion *= gs_nTwoPi * gs_nRadius * gs_nStrength / gs_nNumSamples;
	nAmbientOcclusion = min(1, max(0, 1 - nAmbientOcclusion));

    // Below is directly from the Alchemy SSAO
	// Bilateral box-filter over a quad for free, respecting depth edges
	// (the difference that this makes is subtle)
    if (abs(ddx(nCenterPosition.z)) < 0.02)
    {
		nAmbientOcclusion -= ddx(nAmbientOcclusion) * ((nPixelCoord.x & 1) - 0.5);
	}

	if (abs(ddy(nCenterPosition.z)) < 0.02)
    {
		nAmbientOcclusion -= ddy(nAmbientOcclusion) * ((nPixelCoord.y & 1) - 0.5);
	}
                
    return float4(0,0,0,nAmbientOcclusion);
}

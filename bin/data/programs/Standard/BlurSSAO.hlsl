#include "GEKGlobal.h"
#include "GEKTypes.h"

Texture2D           gs_pAlbedoBuffer        : register(t1);
Texture2D<float>    gs_pDepthBuffer         : register(t2);
Texture2D           gs_pOutputBuffer        : register(t3);

static const float gs_aGuassian[][7] = 
{
    { 0.356642, 0.239400, 0.072410, 0.009869, 0,        0,        0        },
    { 0.398943, 0.241971, 0.053991, 0.004432, 0.000134, 0,        0        }, // stddev = 1.0
    { 0.153170, 0.144893, 0.122649, 0.092902, 0.062970, 0,        0        }, // stddev = 2.0
    { 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 }  // stddev = 3.0
};
            
float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    float2 nPixelSize;
    gs_pOutputBuffer.GetDimensions(nPixelSize.x, nPixelSize.y);
    nPixelSize = rcp(nPixelSize);                

	float nCenterDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, kInput.texCoord);
	float4 nCenterColor = gs_pOutputBuffer.Sample(gs_pPointSampler, kInput.texCoord);

	float nTotalWeight = gs_aGuassian[gs_nGuassianDeviation][0];
	float nAmbientOcclusion = nCenterColor.a * nTotalWeight;

	[unroll]
	for (int nSample = -gs_nRadius; nSample <= gs_nRadius; ++nSample)
	{
		// We already handled the zero case above.  This loop should be unrolled and the branch discarded
		if (nSample != 0)
		{
			float2 nSampleCoord = kInput.texCoord + gs_nAxis * (nSample * nPixelSize * gs_nScale);
			float nSampleDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, nSampleCoord);
			float nSampleAmbient = gs_pOutputBuffer.Sample(gs_pPointSampler, nSampleCoord).a;

			// spatial domain: offset gaussian tap
			float nWeight = gs_aGuassian[gs_nGuassianDeviation][abs(nSample)];

			// range domain (the "bilateral" weight). As depth difference increases, decrease weight.
			nWeight *= rcp(gs_nEpsilon + gs_nEdgeSharpness * abs(nCenterDepth - nSampleDepth));

			nAmbientOcclusion += nSampleAmbient * nWeight;
			nTotalWeight += nWeight;
		}
	}

    nAmbientOcclusion = (nAmbientOcclusion / (nTotalWeight + gs_nEpsilon));
    if(gs_bCombine)
    {
        float4 nAlbedo = gs_pAlbedoBuffer.Sample(gs_pPointSampler, kInput.texCoord);
                    
        [branch]
        if(nAlbedo.a == 1)
        {
	        return float4(nCenterColor.xyz, 0);
        }
        else
        {
	        return float4(nCenterColor.xyz * nAmbientOcclusion, 0);
        }
    }
    else
    {
	    return float4(nCenterColor.xyz, nAmbientOcclusion);
    }
}

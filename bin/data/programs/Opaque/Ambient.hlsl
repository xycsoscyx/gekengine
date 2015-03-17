#include "..\gekengine.h"

Texture2D gs_pAlbedoBuffer : register(t1);

float4 MainPixelProgram(in INPUT kInput) : SV_TARGET0
{
	float4 nAlbedo = gs_pAlbedoBuffer.Sample(gs_pPointSampler, kInput.texcoord);

    [branch]
    if (nAlbedo.a < 1.0f)
    {
		nAlbedo *= gs_nAmbientLight;
    }

    return nAlbedo;
}
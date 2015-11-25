#include "gekengine.h"

struct INSTANCE
{
    float3      m_nPosition;
    float       m_nDistance;
    float       m_nAge;
    float       m_nSpin;
    float       m_nSize;
    float4      m_nColor;
};
            
StructuredBuffer<INSTANCE> gs_aInstances    : register(t0);
Texture1D                  gs_pGradient     : register(t1);

struct SOURCEVERTEX
{
	float2 position                         : POSITION;
	float2 texCoord                         : TEXCOORD0;
    uint instance                           : SV_InstanceId;
};

#include "gekplugin.h"

WORLDVERTEX GetWorldVertex(in SOURCEVERTEX kSource)
{
    INSTANCE kInstance = gs_aInstances[kSource.instance];
                
    float nSin, nCos;
    sincos(kInstance.m_nSpin, nSin, nCos);
    nSin *= kInstance.m_nSize;
    nCos *= kInstance.m_nSize;
                
	float3 kXOffSet = mul(float3(nSin, nCos, 0.0f), (float3x3)Camera::viewMatrix);
	float3 kYOffSet = mul(float3(nCos,-nSin, 0.0f), (float3x3)Camera::viewMatrix);

	WORLDVERTEX kVertex;
	kVertex.position      = float4(kInstance.m_nPosition, 1.0f);
	kVertex.position.xyz += (kXOffSet * kSource.position.x);
	kVertex.position.xyz += (kYOffSet * kSource.position.y);
	kVertex.texCoord      = kSource.texCoord;
    kVertex.normal        = mul(float3(0,0,-1), (float3x3)Camera::viewMatrix);
    kVertex.color         = (gs_pGradient.SampleLevel(gs_pLinearSampler, kInstance.m_nAge, 0) * kInstance.m_nColor);
	return kVertex;
}

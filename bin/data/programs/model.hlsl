#include "gekengine.h"

struct INSTANCE
{
    float4x4    m_nMatrix;
    float3      m_nScale;
    float       m_nPadding;
    float4      m_nColor;
};
            
StructuredBuffer<INSTANCE> gs_aInstances    : register(t0);

struct SOURCEVERTEX
{
	float3 position                         : POSITION;
	float2 texcoord                         : TEXCOORD0;
	float3 normal                           : NORMAL0;
    uint instance                           : SV_InstanceId;
};

WORLDVERTEX GetWorldVertex(in SOURCEVERTEX kSource)
{
    INSTANCE kInstance = gs_aInstances[kSource.instance];
    float4x4 nObject = kInstance.m_nMatrix;

	WORLDVERTEX kVertex;
	kVertex.position    = mul(nObject, float4(kSource.position * kInstance.m_nScale, 1.0f));
	kVertex.texcoord    = kSource.texcoord;
	kVertex.normal      = mul(kSource.normal, (float3x3)nObject);
    kVertex.color       = kInstance.m_nColor;
	return kVertex;
}

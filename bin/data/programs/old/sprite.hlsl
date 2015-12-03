#include "gekengine.h"

struct INSTANCE
{
    float3      m_nPosition;
    float       m_nHalfSize;
    float4      m_nColor;
};
            
StructuredBuffer<INSTANCE> gs_aInstances    : register(t0);

struct SOURCEVERTEX
{
	float2 position                         : POSITION;
	float2 texCoord                         : TEXCOORD0;
    uint instance                           : SV_InstanceId;
};

#include "gekplugin.h"

WORLDVERTEX GetWorldVertex(SOURCEVERTEX kSource)
{
    INSTANCE kInstance = gs_aInstances[kSource.instance];
	float3 kXOffSet = mul(float3(kInstance.m_nHalfSize, 0.0, 0.0f), (float3x3)Camera::viewMatrix);
	float3 kYOffSet = mul(float3(0.0,-kInstance.m_nHalfSize, 0.0f), (float3x3)Camera::viewMatrix);

	WORLDVERTEX kVertex;
	kVertex.position      = float4(kInstance.m_nPosition, 1.0f);
	kVertex.position.xyz += (kXOffSet * kSource.position.x);
	kVertex.position.xyz += (kYOffSet * kSource.position.y);
	kVertex.texCoord      = kSource.texCoord;
    kVertex.normal        = mul(float3(0,0,-1), (float3x3)Camera::viewMatrix);
    kVertex.color         = kInstance.m_nColor;
	return kVertex;
}

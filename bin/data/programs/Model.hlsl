#include "GEKEngine.h"

struct Instance
{
    float4x4 transform;
    float3   scale;
    float4   color;
    float    distance;
};
            
StructuredBuffer<Instance> instanceList : register(t0);

struct PluginVertex
{
	float3 position : POSITION;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
    uint   instance : SV_InstanceId;
};

#include "GEKPlugin.h"

WorldVertex getWorldVertex(in PluginVertex pluginVertex)
{
    Instance instance = instanceList[pluginVertex.instance];

    WorldVertex worldVertex;
	worldVertex.position = mul(instance.transform, float4(pluginVertex.position * instance.scale, 1.0f));
	worldVertex.texcoord = pluginVertex.texcoord;
    worldVertex.normal   = mul((float3x3)instance.transform, pluginVertex.normal);
    worldVertex.color    = instance.color;
	return worldVertex;
}

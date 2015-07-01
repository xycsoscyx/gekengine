#include "GEKGlobal.h"

#include "GEKEngine"

struct Instance
{
    float4x4 transform;
    float4   color;
    float3   scale;
    float    distance;
};
            
#include "GEKPlugin"

WorldVertex getWorldVertex(in PluginVertex pluginVertex)
{
    Instance instance = instanceList[pluginVertex.instanceIndex];

    WorldVertex worldVertex;
	worldVertex.position = mul(instance.transform, float4(pluginVertex.position * instance.scale, 1.0f));
	worldVertex.texcoord = pluginVertex.texcoord;
    worldVertex.normal   = mul((float3x3)instance.transform, pluginVertex.normal);
    worldVertex.color    = instance.color;
	return worldVertex;
}

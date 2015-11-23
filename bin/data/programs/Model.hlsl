#include "GEKGlobal.h"

WorldVertex getWorldVertex(in PluginVertex pluginVertex)
{
    WorldVertex worldVertex;
	worldVertex.position = mul(pluginVertex.transform, float4(pluginVertex.position * pluginVertex.scale, 1.0f));
	worldVertex.texCoord = pluginVertex.texCoord;
    worldVertex.normal   = mul((float3x3)pluginVertex.transform, pluginVertex.normal);
    worldVertex.color    = pluginVertex.color;
	return worldVertex;
}

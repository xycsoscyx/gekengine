#include "GEKGlobal.h"

WorldVertex getWorldVertex(PluginVertex pluginVertex)
{
    WorldVertex worldVertex;
	worldVertex.position = mul(pluginVertex.transform, float4(pluginVertex.position * pluginVertex.scale, 1.0));
	worldVertex.texCoord = pluginVertex.texCoord;
    worldVertex.normal   = mul(pluginVertex.transform, float4(pluginVertex.normal, 0.0)).xyz;
    worldVertex.color    = pluginVertex.color;
	return worldVertex;
}

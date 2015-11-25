#include "GEKGlobal.h"

WorldVertex getWorldVertex(in PluginVertex pluginVertex)
{
    WorldVertex worldVertex;
	worldVertex.position = mul(pluginVertex.transform, float4(pluginVertex.position * pluginVertex.scale, 1.0f));
	worldVertex.texCoord = pluginVertex.texCoord;
    worldVertex.normal   = mul(pluginVertex.transform, float4(pluginVertex.normal, 0.0f)).xyz;
    worldVertex.color    = pluginVertex.color;
	return worldVertex;
}

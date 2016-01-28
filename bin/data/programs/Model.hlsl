#include "GEKGlobal.h"

namespace Model
{
    cbuffer buffer : register(b1)
    {
        float4x4 transform;
        float4 color;
        float4 scale;
    };
};

WorldVertex getWorldVertex(PluginVertex pluginVertex)
{
    WorldVertex worldVertex;
	worldVertex.position = mul(Model::transform, float4(pluginVertex.position * Model::scale.xyz, 1.0));
	worldVertex.texCoord = pluginVertex.texCoord;
    worldVertex.normal   = mul(Model::transform, float4(pluginVertex.normal, 0.0)).xyz;
    worldVertex.color = 1;//Model::color;
	return worldVertex;
}

#include "GEKGlobal.hlsl"

namespace Model
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
        float4 c;
        float4 scale;
    };
};

ViewVertex getViewVertex(PluginVertex pluginVertex)
{
    ViewVertex viewVertex;
    viewVertex.position = mul(Model::transform, float4(pluginVertex.position * Model::scale.xyz, 1.0)).xyz;
    viewVertex.normal = mul((float3x3)Model::transform, pluginVertex.normal);
    viewVertex.texCoord = pluginVertex.texCoord;
    viewVertex.color = Model::c;
    return viewVertex;
}

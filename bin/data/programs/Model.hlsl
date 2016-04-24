#include "GEKGlobal.hlsl"

namespace Model
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
        float4 color;
        float4 scale;
    };
};

ViewVertex getViewVertex(PluginVertex pluginVertex)
{
    float4x4 transform = mul(Model::transform, Camera::viewMatrix);

    ViewVertex viewVertex;
    viewVertex.position = mul(transform, float4(pluginVertex.position * Model::scale.xyz, 1.0)).xyz;
    viewVertex.normal = mul(transform, pluginVertex.normal);
    viewVertex.texCoord = pluginVertex.texCoord;
    viewVertex.color = Model::color;
    return viewVertex;
}

#include "GEKGlobal.h"

namespace Particles
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
    };

    struct Instance
    {
        float3 position;
        float rotation;
        float4 color;
        float2 size;
        float buffer[2];
    };

    StructuredBuffer<Instance> list : register(t0);
};

WorldVertex getWorldVertex(PluginVertex pluginVertex)
{
    uint particleIndex = pluginVertex.vertexIndex / 4;
    uint cornerIndex = pluginVertex.vertexIndex % 4;

    Particles::Instance instance = Particles::list[particleIndex];

    // calculate the position of the vertex
    float4 position;
    position.x = (cornerIndex % 2) ? 1.0 : -1.0;
    position.y = (cornerIndex & 2) ? -1.0 : 1.0;
    position.z = 0.0;
    position.w = 1.0;
    position.xy *= instance.size;
    position = mul(Particles::transform, position);

    WorldVertex worldVertex;
    worldVertex.position = position;
    worldVertex.normal = float3(0.0, 0.0, -1.0);
    worldVertex.color = instance.color;

    // texture coordinate
    worldVertex.texCoord.x = (cornerIndex % 2) ? 1.0 : 0.0;
    worldVertex.texCoord.y = (cornerIndex & 2) ? 1.0 : 0.0;

    return worldVertex;
}

#include "GEKGlobal.h"

namespace Particles
{
    struct Instance
    {
        float3 position;
        float life;
        float style;
        float buffer[3];
    };

    StructuredBuffer<Instance> list : register(t0);
    Texture2D<float4> colorMap : register(t1);
    Texture2D<float> sizeMap : register(t2);
};

WorldVertex getWorldVertex(PluginVertex pluginVertex)
{
    uint particleIndex = pluginVertex.vertexIndex / 4;
    uint cornerIndex = pluginVertex.vertexIndex % 4;

    Particles::Instance instance = Particles::list[particleIndex];

    // calculate the position of the vertex
    float3 position = instance.position;
    position.x += (cornerIndex % 2) ? 1.0 : -1.0;
    position.y += (cornerIndex & 2) ? -1.0 : 1.0;
    position *= Particles::sizeMap.Sample(Global::linearWrapSampler, float2(instance.life, instance.style));

    WorldVertex worldVertex;
    worldVertex.position = float4(position, 1.0);
    worldVertex.normal = float3(0.0, 0.0, -1.0);
    worldVertex.color = Particles::colorMap.Sample(Global::linearWrapSampler, float2(instance.life, instance.style));

    // texture coordinate
    worldVertex.texCoord.x = (cornerIndex % 2) ? 1.0 : 0.0;
    worldVertex.texCoord.y = (cornerIndex & 2) ? 1.0 : 0.0;

    return worldVertex;
}

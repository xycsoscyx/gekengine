#include "GEKGlobal.h"

namespace Particles
{
    struct Instance
    {
        float3 position;
        float3 velocity;
        float life;
        float style;
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
    float3 position;
    position.x = (cornerIndex % 2) ? 1.0 : -1.0;
    position.y = (cornerIndex & 2) ? -1.0 : 1.0;
    position.z = 0.0f;
    position = mul(Camera::viewMatrix, position);
    position *= Particles::sizeMap[float2(instance.life, instance.style)];
    position += instance.position;

    WorldVertex worldVertex;
    worldVertex.position = float4(position, 1.0);
    worldVertex.normal = float3(0.0, 0.0, -1.0);
    worldVertex.color = Particles::colorMap[float2(instance.life, instance.style)];

    // texture coordinate
    worldVertex.texCoord.x = (cornerIndex % 2) ? 1.0 : 0.0;
    worldVertex.texCoord.y = (cornerIndex & 2) ? 1.0 : 0.0;

    return worldVertex;
}

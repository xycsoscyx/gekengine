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

static const uint indexBuffer[6] = 
{
    0, 1, 2,
    1, 3, 2,
};

WorldVertex getWorldVertex(PluginVertex pluginVertex)
{
    uint particleIndex = pluginVertex.vertexIndex / 6;
    uint cornerIndex = indexBuffer[pluginVertex.vertexIndex % 6];

    Particles::Instance instance = Particles::list[particleIndex];

    // calculate the position of the vertex
    float3 position;
    position.x = ((cornerIndex % 2) ?  1.0 : -1.0);
    position.y = ((cornerIndex & 2) ? -1.0 :  1.0);
    position.z = 0.0f;
    position = mul(position, Camera::viewMatrix).xyz;
    position *= Particles::sizeMap.SampleLevel(Global::linearClampSampler, float2(1 - instance.life, instance.style), 0);
    position += instance.position;

    WorldVertex worldVertex;
    worldVertex.position = float4(position, 1.0);
    worldVertex.normal = float3(0.0, 0.0, -1.0);
    worldVertex.color = Particles::colorMap.SampleLevel(Global::linearClampSampler, float2(1 - instance.life, instance.style), 0);

    // texture coordinate
    worldVertex.texCoord.x = ((cornerIndex % 2) ? 1.0 : 0.0);
    worldVertex.texCoord.y = ((cornerIndex & 2) ? 1.0 : 0.0);

    return worldVertex;
}

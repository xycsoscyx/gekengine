#include "GEKGlobal.hlsl"

namespace Particles
{
    struct Instance
    {
        float3 origin;
        float2 offset;
        float age, death;
        float angle;
    };

    StructuredBuffer<Instance> list : register(t0);
    Texture2D<float4> colorMap : register(t1);
};

static const uint indexBuffer[6] = 
{
    0, 1, 2,
    1, 3, 2,
};

ViewVertex getViewVertex(PluginVertex pluginVertex)
{
    uint particleIndex = pluginVertex.vertexIndex / 6;
    uint cornerIndex = indexBuffer[pluginVertex.vertexIndex % 6];

    Particles::Instance instanceData = Particles::list[particleIndex];

    float age = (instanceData.age / instanceData.death);

    ViewVertex viewVertex;
    viewVertex.position.x = ((cornerIndex % 2) ? 1.0 : -1.0);
    viewVertex.position.y = ((cornerIndex & 2) ? -1.0 : 1.0);
    viewVertex.position.z = 0.0f;
    viewVertex.position *= 0.75;

    float3 position;
    float offset = sin(age * Math::Pi);
    position.x = instanceData.origin.x + instanceData.offset.x * offset;
    position.y = instanceData.origin.y + instanceData.age;
    position.z = instanceData.origin.z + instanceData.offset.y * offset;
    viewVertex.position += mul(Camera::viewMatrix, float4(position, 1.0)).xyz;

    viewVertex.normal = float3(0.0, 0.0, -1.0);

    viewVertex.texCoord.x = ((cornerIndex % 2) ? 1.0 : 0.0);
    viewVertex.texCoord.y = ((cornerIndex & 2) ? 1.0 : 0.0);

    viewVertex.color = Particles::colorMap.SampleLevel(Global::linearClampSampler, age, 0);

    return viewVertex;
}

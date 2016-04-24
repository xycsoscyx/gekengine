#include "GEKGlobal.hlsl"

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

ViewVertex getViewVertex(PluginVertex pluginVertex)
{
    uint particleIndex = pluginVertex.vertexIndex / 6;
    uint cornerIndex = indexBuffer[pluginVertex.vertexIndex % 6];

    Particles::Instance instanceData = Particles::list[particleIndex];

    float2 infoCoord;
    infoCoord.x = (1.0 - instanceData.life);
    infoCoord.y = instanceData.style;

    ViewVertex viewVertex;

    viewVertex.position.x = ((cornerIndex % 2) ? 1.0 : -1.0);
    viewVertex.position.y = ((cornerIndex & 2) ? -1.0 : 1.0);
    viewVertex.position.z = 0.0f;
    viewVertex.position *= Particles::sizeMap.SampleLevel(Global::linearClampSampler, infoCoord, 0);
    viewVertex.position += mul(Camera::viewMatrix, float4(instanceData.position, 1.0));

    viewVertex.normal = float3(0.0, 0.0, -1.0);

    viewVertex.texCoord.x = ((cornerIndex % 2) ? 1.0 : 0.0);
    viewVertex.texCoord.y = ((cornerIndex & 2) ? 1.0 : 0.0);

    viewVertex.color = Particles::colorMap.SampleLevel(Global::linearClampSampler, infoCoord, 0);

    return viewVertex;
}

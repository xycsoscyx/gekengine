#include <GEKGlobal.hlsl>

#include GEKEngine

namespace Sprite
{
    struct Data
    {
        float3 position;
        float3 velocity;
        float angle;
        float torque;
        float halfSize;
        float age;
        float4 color;
        float2 texScale;
    };

    StructuredBuffer<Data> list : register(t0);
    Texture2D<float4> colorMap : register(t1);
};

static const uint indexBuffer[6] = 
{
    0, 1, 2,
    1, 3, 2,
};

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    uint spriteIndex = (inputVertex.vertexIndex / 6);
    uint cornerIndex = indexBuffer[inputVertex.vertexIndex % 6];
    Sprite::Data spriteData = Sprite::list[spriteIndex];

    float sinAngle, cosAngle;
    sincos(spriteData.angle, sinAngle, cosAngle);
    float3x3 angle = float3x3(cosAngle, sinAngle, 0.0,
                             -sinAngle, cosAngle, 0.0,
                              0.0, 0.0, 1.0);

    float3 edge;
    float2 normal;
    normal.x = edge.x = ((cornerIndex % 2) ? 1.0 : -1.0);
    normal.y = edge.y = ((cornerIndex & 2) ? -1.0 : 1.0);
    edge.z = 0.0f;
	edge = mul(angle, (edge * spriteData.halfSize));

    OutputVertex outputVertex;
    outputVertex.position = (edge + mul(Camera::viewMatrix, float4(spriteData.position, 1.0)).xyz);
    outputVertex.normal = normalize(float3(normal.xy, -1.0));
    outputVertex.texCoord.x = ((cornerIndex % 2) ? 1.0 : 0.0);
    outputVertex.texCoord.y = ((cornerIndex & 2) ? 1.0 : 0.0);
    outputVertex.texCoord *= spriteData.texScale;
    outputVertex.color = spriteData.color;
    return getProjection(outputVertex);
}

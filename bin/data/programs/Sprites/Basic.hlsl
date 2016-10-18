#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

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
        float life;
        uint frames;
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
    float age = (spriteData.age / spriteData.life);
    uint frameIndex = floor(age * pow(spriteData.frames, 2.0));
    float2 texCoord00 = float2((frameIndex % spriteData.frames),
                               (frameIndex / spriteData.frames)) / float(spriteData.frames);
    float2 texCoord11 = (texCoord00 + (1.0 / float(spriteData.frames)));

    float sinAngle, cosAngle;
    sincos(spriteData.angle, sinAngle, cosAngle);

    float2 normal;
    normal.x = ((cornerIndex % 2) ? +1.0 : -1.0);
    normal.y = ((cornerIndex & 2) ? -1.0 : +1.0);

    float3 edge;
    edge.x = ((normal.x *  cosAngle) + (normal.y * sinAngle)) * spriteData.halfSize;
    edge.y = ((normal.x * -sinAngle) + (normal.y * cosAngle)) * spriteData.halfSize;
    edge.z = 0.0;

    OutputVertex outputVertex;
    outputVertex.position = (edge + mul(Camera::viewMatrix, float4(spriteData.position, 1.0)).xyz);
    outputVertex.normal = normalize(float3(normal.xy, -1.0));
    outputVertex.texCoord.x = ((cornerIndex % 2) ? texCoord11.x : texCoord00.x);
    outputVertex.texCoord.y = ((cornerIndex & 2) ? texCoord11.y : texCoord00.y);
    return getProjection(outputVertex);
}

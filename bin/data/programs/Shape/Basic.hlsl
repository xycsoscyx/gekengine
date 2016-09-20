#include <GEKGlobal.hlsl>

#include GEKEngine

namespace Model
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
    };
};

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    OutputVertex outputVertex;
    outputVertex.position = mul(Model::transform, float4(inputVertex.position, 1.0)).xyz;
    outputVertex.normal = mul((float3x3)Model::transform, inputVertex.normal);
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

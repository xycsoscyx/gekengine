#include <GEKGlobal.hlsl>

#include GEKEngine

namespace Shape
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
    };
};

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    OutputVertex outputVertex;
    outputVertex.position = mul(Shape::transform, float4(inputVertex.position, 1.0)).xyz;
    outputVertex.tangent = mul(Shape::transform, float4(inputVertex.tangent, 0.0)).xyz;
    outputVertex.biTangent = mul(Shape::transform, float4(inputVertex.biTangent, 0.0)).xyz;
    outputVertex.normal = mul(Shape::transform, float4(inputVertex.normal, 0.0)).xyz;
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

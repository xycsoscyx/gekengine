#include "GEKGlobal.hlsl"

#include "GEKVisual"

namespace Model
{
    cbuffer Constants : register(b4)
    {
        float4x4 transform;
        float4 baseColor;
        float4 scale;
    };
};

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    OutputVertex outputVertex;
    outputVertex.position = mul(Model::transform, float4(inputVertex.position * Model::scale.xyz, 1.0)).xyz;
    outputVertex.normal = mul((float3x3)Model::transform, inputVertex.normal);
    outputVertex.texCoord = inputVertex.texCoord;
    outputVertex.color = Model::baseColor;
    return getProjection(outputVertex);
}

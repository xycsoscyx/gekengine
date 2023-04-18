#include <GEKGlobal.hlsl>

#include <GEKEngine>

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    float3x3 directionTransform = (float3x3)inputVertex.transform;
    float3 biTangent = cross(inputVertex.normal, inputVertex.tangent.xyz) * inputVertex.tangent.w;

    OutputVertex outputVertex;
    outputVertex.position = mul(inputVertex.transform, float4(inputVertex.position, 1.0)).xyz;
	outputVertex.tangent = mul(directionTransform, inputVertex.tangent.xyz);
    outputVertex.biTangent = mul(directionTransform, biTangent);
    outputVertex.normal = mul(directionTransform, inputVertex.normal);
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

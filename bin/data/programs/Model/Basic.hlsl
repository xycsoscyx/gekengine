#include <GEKGlobal.hlsl>

#include GEKEngine

OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    OutputVertex outputVertex;
    outputVertex.position = mul(inputVertex.transform, float4(inputVertex.position, 1.0)).xyz;
	outputVertex.tangent = mul((float3x3)inputVertex.transform, inputVertex.tangent);
	outputVertex.biTangent = mul((float3x3)inputVertex.transform, inputVertex.biTangent);
	outputVertex.normal = mul((float3x3)inputVertex.transform, inputVertex.normal);
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

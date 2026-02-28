#include <GEKGlobal.slang>

#include <GEKEngine>

[shader("vertex")]
OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    float3x3 directionTransform = (float3x3)inputVertex.transform;
    float3 biTangent = cross(inputVertex.normal, inputVertex.tangent.xyz) * inputVertex.tangent.w;

    OutputVertex outputVertex;
    outputVertex.position = mul(float4(inputVertex.position, 1.0), inputVertex.transform).xyz;
	outputVertex.tangent = mul(inputVertex.tangent.xyz, directionTransform);
    outputVertex.biTangent = mul(biTangent, directionTransform);
    outputVertex.normal = mul(inputVertex.normal, directionTransform);
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

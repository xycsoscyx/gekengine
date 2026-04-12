#include <GEKGlobal.slang>

#include <GEKEngine>

[shader("vertex")]
OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    float3x3 directionTransform = (float3x3)inputVertex.transform;
    float3 tangent = normalize(mul(inputVertex.tangent.xyz, directionTransform));
    float3 normal = normalize(mul(inputVertex.normal, directionTransform));
    float3 biTangent = normalize(cross(normal, tangent) * inputVertex.tangent.w);

    OutputVertex outputVertex;
    outputVertex.position = mul(float4(inputVertex.position, 1.0), inputVertex.transform).xyz;
	outputVertex.tangent = tangent;
    outputVertex.biTangent = biTangent;
    outputVertex.normal = normal;
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

#include <GEKGlobal.slang>

#include <GEKEngine>

[shader("vertex")]
OutputVertex mainVertexProgram(InputVertex inputVertex)
{
    float3x3 directionTransform = (float3x3)inputVertex.transform;
    float transformHandedness = (determinant(directionTransform) < 0.0f) ? -1.0f : 1.0f;

    float3 normal = mul(inputVertex.normal, directionTransform);
    float normalLengthSquared = dot(normal, normal);
    if (normalLengthSquared <= 1.0e-10f)
    {
        normal = float3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        normal *= rsqrt(normalLengthSquared);
    }

    float3 tangent = mul(inputVertex.tangent.xyz, directionTransform);
    tangent -= (normal * dot(tangent, normal));
    float tangentLengthSquared = dot(tangent, tangent);
    if (tangentLengthSquared <= 1.0e-10f)
    {
        float3 axis = (abs(normal.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
        tangent = normalize(cross(axis, normal));
    }
    else
    {
        tangent *= rsqrt(tangentLengthSquared);
    }

    float3 biTangent = cross(normal, tangent) * (inputVertex.tangent.w * transformHandedness);
    float biTangentLengthSquared = dot(biTangent, biTangent);
    if (biTangentLengthSquared > 1.0e-10f)
    {
        biTangent *= rsqrt(biTangentLengthSquared);
    }

    OutputVertex outputVertex;
    outputVertex.position = mul(float4(inputVertex.position, 1.0), inputVertex.transform).xyz;
	outputVertex.tangent = float4(tangent, inputVertex.tangent.w * transformHandedness);
    outputVertex.biTangent = biTangent;
    outputVertex.normal = normal;
    outputVertex.texCoord = inputVertex.texCoord;
    return getProjection(outputVertex);
}

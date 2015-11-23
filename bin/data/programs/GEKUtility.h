float3x3 getCoTangentFrame(float3 position, float3 normal, float2 texCoord)
{
    // get edge vectors of the pixel triangle
    float3 positionDX = ddx(position);
    float3 positionDY = ddy(position);
    float2 texCoordDX = ddx(texCoord);
    float2 texCoordDY = ddy(texCoord);

    // solve the linear system
    float3 perpendicularDX = cross(normal, positionDX);
    float3 perpendicularDY = cross(positionDY, normal);
    float3 tangent = perpendicularDY * texCoordDX.x + perpendicularDX * texCoordDY.x;
    float3 biTangent = perpendicularDY * texCoordDX.y + perpendicularDX * texCoordDY.y;

    // construct a scale-invariant frame 
    float reciprocal = rsqrt(max(dot(tangent, tangent), dot(biTangent, biTangent)));
    return float3x3(normalize(tangent * reciprocal), normalize(biTangent * reciprocal), normal);
}

half2 encodeNormal(half3 normal)
{
    return (normalize(normal.xy) * sqrt(normal.z * 0.5f + 0.5f));
}

half3 decodeNormal(half2 encoded)
{
    half z = dot(encoded.xy, encoded.xy) * 2.0f - 1.0f;
    return half3(normalize(encoded.xy) * sqrt(1 - z * z), z);
}

float3 getViewPosition(float2 texCoord, float depth)
{
    float2 adjustedCoord = texCoord;
    adjustedCoord.y = (1.0f - adjustedCoord.y);
    adjustedCoord.xy = (adjustedCoord.xy * 2.0f - 1.0f);
    return (float3((adjustedCoord * Camera::fieldOfView), 1.0f) * depth * Camera::maximumDistance);
}

float3x3 getCoTangentFrame1(float3 position, float3 normal, float2 texCoord)
{
    // get edge vectors of the pixel triangle
    float3 positionDDX = ddx(position);
    float3 positionDDY = ddy(position);
    float2 texCoordDDX = ddx(texCoord);
    float2 texCoordDDY = ddy(texCoord);

    // solve the linear system
    float3 perpendicularDX = cross(normal, positionDDX);
    float3 perpendicularDY = cross(positionDDY, normal);
    float3 tangent =   perpendicularDY * texCoordDDX.x + perpendicularDX * texCoordDDY.x;
    float3 biTangent = perpendicularDY * texCoordDDX.y + perpendicularDX * texCoordDDY.y;

    // construct a scale-invariant frame 
    float reciprocal = rsqrt(max(dot(tangent, tangent), dot(biTangent, biTangent)));
    return float3x3(normalize(tangent * reciprocal), normalize(biTangent * reciprocal), normal);
}

float3x3 getCoTangentFrame(float3 position, float3 normal, float2 texCoord)
{
    // compute derivations of the world position
    float3 positionDDX = ddx(position);
    float3 positionDDY = ddy(position);

    // compute derivations of the texture coordinate
    float2 texCoordDDX = ddx(texCoord);
    float2 texCoordDDY = ddy(texCoord);

    // compute initial tangent and bi-tangent
    float3 tangent =   normalize(texCoordDDY.y * positionDDX - texCoordDDX.y * positionDDY);
    float3 biTangent = normalize(texCoordDDY.x * positionDDX - texCoordDDX.x * positionDDY); // sign inversion

    // get new tangent from a given mesh normal
    float3 factor = cross(normal, tangent);
    tangent = cross(factor, normal);
    tangent = normalize(tangent);

    // get updated bi-tangent
    factor = cross(biTangent, normal);
    biTangent = cross(normal, factor);
    biTangent = normalize(biTangent);
    return float3x3(tangent, biTangent, normal);
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

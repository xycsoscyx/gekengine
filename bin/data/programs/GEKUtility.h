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
    half f = sqrt(8 * normal.z + 8);
    return normal.xy / f + 0.5;
}

half3 decodeNormal(half2 encodedNormal)
{
    half2 fixedNormal = encodedNormal * 4 - 2;
    half f = dot(fixedNormal, fixedNormal);
    half g = sqrt(1 - f / 4);
    return half3(fixedNormal*g, 1 - f / 2);
}

float3 getViewPosition(float2 texCoord, float nDepth)
{
    texCoord.y = 1 - texCoord.y;
    texCoord = (texCoord * 2 - 1);
    float3 viewVector = float3((texCoord * Camera::fieldOfView), 1.0);
    return (viewVector * nDepth * Camera::maximumDistance);
}

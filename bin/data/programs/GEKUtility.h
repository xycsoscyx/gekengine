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

half2 encodeNormal(half3 n)
{
    return (normalize(n.xy) * sqrt(n.z * 0.5 + 0.5));
}

half3 decodeNormal(half2 enc)
{
    half z = dot(enc.xy, enc.xy) * 2 - 1;
    return half3(normalize(enc.xy) * sqrt(1 - z * z), z);
}

float3 getViewPosition(float2 texCoord, float nDepth)
{
    texCoord.y = 1 - texCoord.y;
    texCoord = (texCoord * 2 - 1);
    float3 viewVector = float3((texCoord * Camera::fieldOfView), 1.0);
    return (viewVector * nDepth * Camera::maximumDistance);
}

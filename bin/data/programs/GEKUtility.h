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
    half2 encodedNormal = (normalize(normal.xy) * (sqrt((-normal.z * 0.5) + 0.5)));
    encodedNormal = ((encodedNormal * 0.5) + 0.5);
    return encodedNormal;
}

half3 decodeNormal(half2 encodedNormal)
{
    half4 normal = half4((encodedNormal * 2 - 1), 1, -1);
    half length = dot(normal.xyz, -normal.xyw);
    normal.z = length;
    normal.xy *= sqrt(length);
    return ((normal.xyz * 2.0f) + half3(0, 0, -1));
}

float3 getViewPosition(float2 texCoord, float nDepth)
{
    texCoord.y = 1 - texCoord.y;
    texCoord = (texCoord * 2 - 1);
    float3 viewVector = float3((texCoord * Camera::fieldOfView), 1.0);
    return (viewVector * nDepth * Camera::maximumDistance);
}

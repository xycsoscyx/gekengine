float sqr(float x)
{
    return x*x;
}

float3x3 getCoTangentFrame(float3 position, float3 normal, float2 texCoord)
{
    normal = normalize(normal);

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
    return float3x3(normalize(tangent * reciprocal), normalize(-biTangent * reciprocal), normal);
}

#define _ENCODE_NORMALS 1

#if _ENCODE_NORMALS
    half2 encodeNormal(float3 n)
    {
        half scale = 1.7777;
        half2 enc = n.xy / (n.z + 1);
        enc /= scale;
        enc = enc*0.5 + 0.5;
        return half4(enc, 0, 0);
    }

    float3 decodeNormal(half2 enc)
    {
        half scale = 1.7777;
        half3 nn = half3(enc * 2 * scale - scale, 1.0f);
        half g = 2.0 / dot(nn.xyz, nn.xyz);
        half3 n;
        n.xy = g*nn.xy;
        n.z = g - 1;
        return n;
    }
#else
    half3 encodeNormal(float3 normal) { return normal; };
    float3 decodeNormal(half3 normal) { return normal; };
#endif

float3 getViewPosition(float2 texCoord, float depth)
{
    float2 adjustedCoord = texCoord;
    adjustedCoord.y = (1.0f - adjustedCoord.y);
    adjustedCoord.xy = (adjustedCoord.xy * 2.0f - 1.0f);
    return (float3((adjustedCoord * Camera::fieldOfView), 1.0f) * depth * Camera::maximumDistance);
}

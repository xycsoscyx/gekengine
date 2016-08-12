float square(float x)
{
    return (x * x);
}

float cube(float x)
{
    return (x * x * x);
}

float random(int2 position, float time = 1.0)
{
    return (61.111231231 * time + (9.2735171213125 * position.x + -7.235171213125 * position.y + 1.53713171123412415411 * (position.x ^ position.y)));
}

float maxComponent(float3 value)
{
    return max(value.x, max(value.y, value.z));
}

float minComponent(float3 value)
{
    return min(value.x, min(value.y, value.z));
}

float getLuminance(float3 color)
{
    float luminance = max(dot(color, float3(0.299, 0.587, 0.114)), Math::Epsilon);
    if (isinf(luminance))
    {
        // maximum float
        luminance = asfloat(0x7F7FFFFF);
    }
    
    return luminance;
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

float getLinearDepthFromSample(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    depthSample = 2.0 * Camera::nearClip * Camera::farClip / (Camera::farClip + Camera::nearClip - depthSample * (Camera::farClip - Camera::nearClip));
    return depthSample;
}

float3 getPositionFromLinearDepth(float2 texCoord, float linearDepth)
{
    float2 adjustedCoord = texCoord;
    adjustedCoord.y = (1.0 - adjustedCoord.y);
    adjustedCoord = (adjustedCoord * 2.0 - 1.0);
    return (float3((adjustedCoord * Camera::fieldOfView), 1.0) * linearDepth);
}

float3 getPositionFromSample(float2 texCoord, float depthSample)
{
    return getPositionFromLinearDepth(texCoord, getLinearDepthFromSample(depthSample));
}

// using stereograpgic projection
// http://aras-p.info/texts/CompactNormalStorage.html
half2 encodeNormal(float3 n)
{
    half scale = 1.7777;
    half2 enc = (n.z == -1.0 ? 0.0 : n.xy / (n.z + 1.0));
    enc /= scale;
    enc = enc * 0.5 + 0.5;
    return enc;
}

float3 decodeNormal(half2 enc)
{
    half scale = 1.7777;
    half3 nn = half3(enc * 2.0 * scale - scale, 1.0);
    float denom = dot(nn.xyz, nn.xyz);
    half g = (denom == 0.0 ? 0.0 : 2.0 / denom);
    half3 n;
    n.xy = g*nn.xy;
    n.z = g - 1.0;
    return n;
}

uint packFloat4(float4 value)
{
    value = min(max(value, 0.0), 1.0);
    value = value * 255.0 + 0.5;
    value = floor(value);
    return (((uint)value.x) |
           (((uint)value.y) << 8) |
           (((uint)value.z) << 16) |
           (((uint)value.w) << 24));
}

float4 unpackFloat4(uint value)
{
    return float4((float)(value & 0x000000ff) / 255.0,
                  (float)((value >> 8) & 0x000000ff) / 255.0,
                  (float)((value >> 16) & 0x000000ff) / 255.0,
                  (float)((value >> 24) & 0x000000ff) / 255.0);
}

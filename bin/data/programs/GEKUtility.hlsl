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

// http://aras-p.info/texts/CompactNormalStorage.html
// http://jcgt.org/published/0003/02/01/paper.pdf

// Returns ±1
float2 signNotZero(float2 v)
{
    return float2((v.x >= 0.0) ? +1.0 : -1.0, (v.y >= 0.0) ? +1.0 : -1.0);
}

// Assume normalized input. Output is on [-1, 1] for each component.
float2 encodeNormal(float3 v)
{
    // Project the sphere onto the octahedron, and then onto the xy plane
    float2 p = v.xy * (1.0 / (abs(v.x) + abs(v.y) + abs(v.z)));
    // Reflect the folds of the lower hemisphere over the diagonals
    return (v.z <= 0.0) ? ((1.0 - abs(p.yx)) * signNotZero(p)) : p;
}

float3 decodeNormal(float2 e)
{
    float3 v = float3(e.xy, 1.0 - abs(e.x) - abs(e.y));
    if (v.z < 0) v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    return normalize(v);
}

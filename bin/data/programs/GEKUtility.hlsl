float getRandom(int2 position, float time = 1.0)
{
    return (61.111231231 * time + (9.2735171213125 * position.x + -7.235171213125 * position.y + 1.53713171123412415411 * (position.x ^ position.y)));
}

// by Morgan McGuire https://www.shadertoy.com/view/4dS3Wd
// All noise functions are designed for values on integer scale.
// They are tuned to avoid visible periodicity for both positive and
// negative coordinates witha few orders of magnitude.
float getHash(float n)
{
	return frac(sin(n) * 1e4);
}

float getHash(float2 p)
{
	return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float getNoise(float2 x)
{
    const float2 i = floor(x);
    const float2 f = frac(x);

	// Four corners 2D of a tile
    const float a = getHash(i);
    const float b = getHash(i + float2(1.0, 0.0));
    const float c = getHash(i + float2(0.0, 1.0));
    const float d = getHash(i + float2(1.0, 1.0));

	// Simple 2D lerp using smoothstep envelope between the values.
	// return float3(lerp(lerp(a, b, smoothstep(0.0, 1.0, f.x)),
	//			lerp(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps smoothstep and common subexpressions
	// optimized away.
    const float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float getMaximum(float3 value)
{
    return max(value.x, max(value.y, value.z));
}

float getMinimum(float3 value)
{
    return min(value.x, min(value.y, value.z));
}

float getMaximum(float4 value)
{
    return max(value.x, max(value.y, max(value.z, value.w)));
}

float getMinimum(float4 value)
{
    return min(value.x, min(value.y, min(value.z, value.w)));
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
    const float3 positionDDX = ddx(position);
    const float3 positionDDY = ddy(position);
    const float2 texCoordDDX = ddx(texCoord);
    const float2 texCoordDDY = ddy(texCoord);

    // solve the linear system
    const float3 perpendicularDX = cross(normal, positionDDX);
    const float3 perpendicularDY = cross(positionDDY, normal);
    const float3 tangent =   perpendicularDY * texCoordDDX.x + perpendicularDX * texCoordDDY.x;
    const float3 biTangent = perpendicularDY * texCoordDDX.y + perpendicularDX * texCoordDDY.y;

    // construct a scale-invariant frame 
    const float reciprocal = rsqrt(max(dot(tangent, tangent), dot(biTangent, biTangent)));
    return float3x3(normalize(tangent * reciprocal), normalize(-biTangent * reciprocal), normal);
}

float getLinearDepthFromSample(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    depthSample = 2.0 * Camera::NearClip * Camera::FarClip / (Camera::FarClip + Camera::NearClip - depthSample * (Camera::FarClip - Camera::NearClip));
    return depthSample;
}

float3 getPositionFromLinearDepth(float2 texCoord, float linearDepth)
{
    float2 adjustedCoord = texCoord;
    adjustedCoord.y = (1.0 - adjustedCoord.y);
    adjustedCoord = (adjustedCoord * 2.0 - 1.0);
    return (float3((adjustedCoord * Camera::FieldOfView), 1.0) * linearDepth);
}

float3 getPositionFromSample(float2 texCoord, float depthSample)
{
    return getPositionFromLinearDepth(texCoord, getLinearDepthFromSample(depthSample));
}

// http://aras-p.info/texts/CompactNormalStorage.html
// http://jcgt.org/published/0003/02/01/paper.pdf
float3 getEncodedNormal(float3 n)
{
    return n + 1.0 * 0.5;
}

float3 getDecodedNormal(float3 n)
{
    return n * 2.0 - 1.0;
}
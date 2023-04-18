float square(float value)
{
    return (value * value);
}

float2 square(float2 value)
{
    return float2(square(value.x), square(value.y));
}

float cube(float value)
{
    return (value * value * value);
}

float GetRandom(int2 position, float time = 1.0)
{
    return (61.111231231 * time + (9.2735171213125 * position.x + -7.235171213125 * position.y + 1.53713171123412415411 * (position.x ^ position.y)));
}

// by Morgan McGuire https://www.shadertoy.com/view/4dS3Wd
// All noise functions are designed for values on integer scale.
// They are tuned to avoid visible periodicity for both positive and
// negative coordinates witha few orders of magnitude.
float GetHash(float n)
{
	return frac(sin(n) * 1e4);
}

float GetHash(float2 p)
{
	return frac(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float GetNoise(float2 x)
{
    const float2 i = floor(x);
    const float2 f = frac(x);

	// Four corners 2D of a tile
    const float a = GetHash(i);
    const float b = GetHash(i + float2(1.0, 0.0));
    const float c = GetHash(i + float2(0.0, 1.0));
    const float d = GetHash(i + float2(1.0, 1.0));

	// Simple 2D lerp using smoothstep envelope between the values.
	// return float3(lerp(lerp(a, b, smoothstep(0.0, 1.0, f.x)),
	//			lerp(c, d, smoothstep(0.0, 1.0, f.x)),
	//			smoothstep(0.0, 1.0, f.y)));

	// Same code, with the clamps smoothstep and common subexpressions
	// optimized away.
    const float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float GetMaximum(float3 value)
{
    return max(value.x, max(value.y, value.z));
}

float GetMinimum(float3 value)
{
    return min(value.x, min(value.y, value.z));
}

float GetMaximum(float4 value)
{
    return max(value.x, max(value.y, max(value.z, value.w)));
}

float GetMinimum(float4 value)
{
    return min(value.x, min(value.y, min(value.z, value.w)));
}

float GetLuminance(float3 color)
{
    float luminance = max(dot(color, float3(0.299, 0.587, 0.114)), Math::Epsilon);
    if (isinf(luminance))
    {
        // maximum float
        luminance = asfloat(0x7F7FFFFF);
    }
    
    return luminance;
}

float3x3 GetCoTangentFrame(float3 position, float3 normal, float2 texCoord)
{
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx(position);
    float3 dp2 = ddy(position);
    float2 duv1 = ddx(texCoord);
    float2 duv2 = ddy(texCoord);

    // solve the linear system
    float3 dp2perp = cross(dp2, normal);
    float3 dp1perp = cross(normal, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame
    float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
    return transpose(float3x3(T * invmax, B * invmax, normal));
}

float GetLinearDepthFromSampleDepth(float depthSample)
{
    depthSample = ((2.0 * depthSample) - 1.0);
    depthSample = (2.0 * Camera::NearClip * Camera::FarClip / (Camera::FarClip + Camera::NearClip - (depthSample * Camera::ClipDistance)));
    return depthSample;
}

float3 GetPositionFromLinearDepth(float2 texCoord, float linearDepth)
{
    float2 adjustedCoord = texCoord;
    adjustedCoord.y = (1.0 - adjustedCoord.y);
    adjustedCoord = (adjustedCoord * 2.0 - 1.0);
    return (float3((adjustedCoord * Camera::FieldOfView), 1.0) * linearDepth);
}

float3 GetPositionFromSampleDepth(float2 texCoord, float depthSample)
{
    return GetPositionFromLinearDepth(texCoord, GetLinearDepthFromSampleDepth(depthSample));
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
float2 OctWrap(float2 normal)
{
    return (1.0 - abs(normal.yx)) * (normal.xy >= 0.0 ? 1.0 : -1.0);
}

float2 GetEncodedNormal(float3 normal)
{
    normal /= (abs(normal.x) + abs(normal.y) + abs(normal.z));
    normal.xy = normal.z >= 0.0 ? normal.xy : OctWrap(normal.xy);
    normal.xy = normal.xy * 0.5 + 0.5;
    return normal.xy;
}

float3 GetDecodedNormal(float2 encoded)
{
    encoded = encoded * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 normal = float3(encoded.x, encoded.y, 1.0 - abs(encoded.x) - abs(encoded.y));
    float sign = saturate(-normal.z);
    normal.xy += normal.xy >= 0.0 ? -sign : sign;
    return normalize(normal);
}

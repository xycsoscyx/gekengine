#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const float width = Shader::targetSize.x;
static const float height = Shader::targetSize.y;
static const float2 texel = Shader::pixelSize;

static const int sampleCount = 4;
static const int ringCount = 7;

static const float focalRange = 10.0;

static const float highlightThreshold = 0.5;
static const float highlightGain = 10.0;

static const float bokehEdgeBias = 0.4;

static const bool enableChromaticAberation = true;
static const float bokehChromaticAberation = 0.5;

static const bool enableNoise = true;
static const float noiseAmount = 0.0001;

static const bool enableDepthBlur = true;
static const float depthBlurSize = 2.0;

/* 
    next part is experimental
    not looking good with small sample and ring count
    looks okay starting from sampleCount = 4, ringCount = 4
 */
static const bool enablePentagon = false;
static const float pentagonFeathering = 0.4;

float getPentagonalShape(float2 texCoord)
{
    static const float scale = float(ringCount) - 1.3;
    static const float4  HS0 = float4(1.0, 0.0, 0.0, 1.0);
    static const float4  HS1 = float4(0.309016994, 0.951056516, 0.0, 1.0);
    static const float4  HS2 = float4(-0.809016994, 0.587785252, 0.0, 1.0);
    static const float4  HS3 = float4(-0.809016994, -0.587785252, 0.0, 1.0);
    static const float4  HS4 = float4(0.309016994, -0.951056516, 0.0, 1.0);
    static const float4  HS5 = float4(0.0, 0.0, 1.0, 1.0);

    float4 texCoordScale = float4(texCoord, scale.xx);
    float4 distance = float4(
        dot(texCoordScale, HS0),
        dot(texCoordScale, HS1),
        dot(texCoordScale, HS2),
        dot(texCoordScale, HS3));
    distance = smoothstep(-pentagonFeathering, pentagonFeathering, distance);

    float inOrOut = -4.0;
    inOrOut += dot(distance, 1.0);

    distance.x = dot(texCoordScale, HS4);
    distance.y = HS5.w - abs(texCoordScale.z);
    distance = smoothstep(-pentagonFeathering, pentagonFeathering, distance);

    inOrOut += distance.x;
    return clamp(inOrOut, 0.0, 1.0);
}

float getBlurredDepth(float2 texCoord) // blurring depth
{
    static const float2 texelOffset = (texel * depthBlurSize);
    float2 offset[9] =
    {
        float2(-texelOffset.x, -texelOffset.y), 
        float2(0.0, -texelOffset.y), 
        float2(texelOffset.x, -texelOffset.y), 
        float2(-texelOffset.x, 0.0), 
        float2(0.0, 0.0), 
        float2(texelOffset.x, 0.0), 
        float2(-texelOffset.x, texelOffset.y), 
        float2(0.0, texelOffset.y), 
        float2(texelOffset.x, texelOffset.y), 
    };

    float kernel[9] = 
    {
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
        (2.0 / 16.0), (4.0 / 16.0), (2.0 / 16.0),
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
    };

    float blurredDepth = 0.0;
    for (int index = 0; index < 9; index++)
    {
        blurredDepth += getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord + offset[index], 0)) * kernel[index];
    }

    return blurredDepth;
}

float3 getFringedColor(float2 texCoord, float blur)
{
    float3 fringedColor = (enableChromaticAberation ? float3(
        Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(0.0, 1.0) * texel * bokehChromaticAberation * blur, 0).r,
        Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(-0.866, -0.5) * texel * bokehChromaticAberation * blur, 0).g,
        Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(0.866, -0.5) * texel * bokehChromaticAberation * blur, 0).b) :
        Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + texel * blur, 0));

    float luminance = getLuminance(fringedColor);
    float thresh = (saturate(luminance - highlightThreshold) * highlightGain);
    return fringedColor + lerp(0.0, fringedColor, thresh * blur);
}

float2 getNoise(in float2 coord)
{
    float noiseX = ((frac(1.0 - coord.x * (width / 2.0)) * 0.25) + (frac(coord.y * (height / 2.0)) * 0.75)) * 2.0 - 1.0;
    float noiseY = ((frac(1.0 - coord.x * (width / 2.0)) * 0.75) + (frac(coord.y * (height / 2.0)) * 0.25)) * 2.0 - 1.0;
    if (enableNoise)
    {
        noiseX = clamp(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0;
        noiseY = clamp(frac(sin(dot(coord, float2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0;
    }

    return float2(noiseX, noiseY);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float focalDepth = Resources::averageFocalDepth[0];
    float sceneDepth = (enableDepthBlur ? getBlurredDepth(inputPixel.texCoord) : getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0)));
    float blurDistance = saturate(abs(sceneDepth - focalDepth) / focalRange);

    float2 noise = getNoise(inputPixel.texCoord) * noiseAmount * blurDistance;
    noise = texel * blurDistance + noise;

    float3 finalColor = Resources::finalCopy.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float totalWeight = 1.0;

    for (float ring = 1.0; ring <= ringCount; ring++)
    {
        float ringsamples = ring * float(sampleCount);
        float step = Math::Tau / float(ringsamples);
        for (float tap = 0.0; tap < ringsamples; tap++)
        {
            float2 offset = float2(
                (cos(tap * step) * ring), 
                (sin(tap * step) * ring));

            float pentagon = (enablePentagon ? getPentagonalShape(offset) : 1.0);
            float weight = (lerp(1.0, (ring / float(ringCount)), bokehEdgeBias) * pentagon);
            finalColor += (getFringedColor(inputPixel.texCoord + (offset * noise), blurDistance) * weight);
            totalWeight += weight;
        }
    }

    return (finalColor / totalWeight);
}
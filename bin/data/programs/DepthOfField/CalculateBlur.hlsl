#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float getPentagonalShape(float2 texCoord)
{
    static const float scale = float(Defines::ringCount) - 1.3;
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
    distance = smoothstep(-Defines::pentagonFeathering, Defines::pentagonFeathering, distance);

    float inOrOut = -4.0;
    inOrOut += dot(distance, 1.0);

    distance.x = dot(texCoordScale, HS4);
    distance.y = HS5.w - abs(texCoordScale.z);
    distance = smoothstep(-Defines::pentagonFeathering, Defines::pentagonFeathering, distance);

    inOrOut += distance.x;
    return clamp(inOrOut, 0.0, 1.0);
}

float getBlurredDepth(float2 texCoord) // blurring depth
{
    static const float2 blurOffset = (Shader::pixelSize * Defines::depthBlurSize);
    float2 offset[9] =
    {
        float2(-blurOffset.x, -blurOffset.y), float2(0.0, -blurOffset.y), float2(blurOffset.x, -blurOffset.y), 
        float2(-blurOffset.x,           0.0), float2(0.0,           0.0), float2(blurOffset.x,           0.0), 
        float2(-blurOffset.x,  blurOffset.y), float2(0.0,  blurOffset.y), float2(blurOffset.x,  blurOffset.y), 
    };

    float kernel[9] = 
    {
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
        (2.0 / 16.0), (4.0 / 16.0), (2.0 / 16.0),
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
    };

    float blurredDepth = 0.0;

    [unroll]
    for (int index = 0; index < 9; index++)
    {
        blurredDepth += (getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord + offset[index], 0)) * kernel[index]);
    }

    return blurredDepth;
}

float getDepth(float2 texCoord)
{
    if (Defines::enableDepthBlur)
    {
        return getBlurredDepth(texCoord);
    }
    else
    {
        return getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord, 0));
    }
}

float3 getChromaticAberation(float2 texCoord, float blur)
{
    float2 offset = (Shader::pixelSize * Defines::bokehChromaticAberation * blur);
    return float3(
        Resources::screenBuffer.SampleLevel(Global::pointSampler, texCoord + float2(0.0, 1.0) * offset, 0).r,
        Resources::screenBuffer.SampleLevel(Global::pointSampler, texCoord + float2(-0.866, -0.5) * offset, 0).g,
        Resources::screenBuffer.SampleLevel(Global::pointSampler, texCoord + float2(0.866, -0.5) * offset, 0).b);
}

float3 getColor(float2 texCoord, float blur)
{
    if (Defines::enableChromaticAberation)
    {
        return getChromaticAberation(texCoord, blur);
    }
    else
    {
        return Resources::screenBuffer.SampleLevel(Global::pointSampler, texCoord + Shader::pixelSize * blur, 0);
    }
}

float3 getCorrectedColor(float2 texCoord, float blur)
{
    float3 color = getColor(texCoord, blur);
    float luminance = getLuminance(color);
    float thresh = (saturate(luminance - Defines::highlightThreshold) * Defines::highlightGain);
    return (color + lerp(0.0, color, (thresh * blur)));
}

float2 getNoise(float2 coord)
{
    if (Defines::enableDithering)
    {
        return float2(
            clamp(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0,
            clamp(frac(sin(dot(coord, float2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0) * Defines::noiseStrength;
    }
    else
    {
        return float2(random(coord.xy), random(coord.yx)) * Defines::noiseStrength;
    }
}

float getModifier(float2 offset)
{
    if (Defines::enablePentagon)
    {
        return getPentagonalShape(offset);
    }
    else
    {
        return 1.0;
    }
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float focalDepth = Resources::averageFocalDepth[0];
    float sceneDepth = getDepth(inputPixel.texCoord);
    float blurDistance = saturate(abs(sceneDepth - focalDepth) / Defines::focalRange);
    float2 noise = ((getNoise(inputPixel.texCoord) * blurDistance) + (Shader::pixelSize * blurDistance));

    float totalWeight = 1.0;
    float3 finalColor = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    
    [unroll]
    for (float ring = 1.0; ring <= Defines::ringCount; ring++)
    {
        float ringsamples = ring * float(Defines::sampleCount);
        float step = Math::Tau / float(ringsamples);

        [unroll]
        for (float tap = 0.0; tap < ringsamples; tap++)
        {
            float2 tapOffset = float2(
                (cos(tap * step) * ring), 
                (sin(tap * step) * ring));

            float modifier = getModifier(tapOffset);
            float weight = (lerp(1.0, (ring / float(Defines::ringCount)), Defines::bokehEdgeBias) * modifier);
            finalColor += (getCorrectedColor(inputPixel.texCoord + (tapOffset * noise), blurDistance) * weight);
            totalWeight += weight;
        }
    }

    finalColor /= totalWeight;
    return finalColor;
}
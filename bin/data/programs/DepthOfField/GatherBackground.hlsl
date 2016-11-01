#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float2 getNoise(float2 coord)
{
    if (Defines::enableDithering)
    {
        return float2(
            saturate(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453)) * 2.0 - 1.0,
            saturate(frac(sin(dot(coord, float2(12.9898, 78.233) * 2.0)) * 43758.5453)) * 2.0 - 1.0) * Defines::noiseStrength;
    }
    else
    {
        return float2(getRandom(coord.xy), getRandom(coord.yx)) * Defines::noiseStrength;
    }
}

float3 getChromaticAberation(float2 texCoord, float blur)
{
    const float2 offset = (Shader::pixelSize * Defines::bokehChromaticAberation * blur);
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
    const float3 color = getColor(texCoord, blur);
    const float luminance = getLuminance(color);
    const float thresh = (saturate(luminance - Defines::highlightThreshold) * Defines::highlightGain);
    return (color + lerp(0.0, color, (thresh * blur)));
}

float getPentagonalShape(float2 texCoord)
{
    static const float scale = float(Defines::ringCount) - 1.3;
    static const float4  HS0 = float4(1.0, 0.0, 0.0, 1.0);
    static const float4  HS1 = float4(0.309016994, 0.951056516, 0.0, 1.0);
    static const float4  HS2 = float4(-0.809016994, 0.587785252, 0.0, 1.0);
    static const float4  HS3 = float4(-0.809016994, -0.587785252, 0.0, 1.0);
    static const float4  HS4 = float4(0.309016994, -0.951056516, 0.0, 1.0);
    static const float4  HS5 = float4(0.0, 0.0, 1.0, 1.0);

    const float4 texCoordScale = float4(texCoord, scale.xx);
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
    return saturate(inOrOut);
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
	const float blurDistance = abs(Resources::circleOfConfusion[inputPixel.screen.xy]);
    const float2 noise = ((getNoise(inputPixel.texCoord) * blurDistance) + (Shader::pixelSize * blurDistance));

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
            const float2 tapOffset = float2((cos(tap * step) * ring), (sin(tap * step) * ring));
            const float modifier = getModifier(tapOffset);
            const float weight = (lerp(1.0, (ring / float(Defines::ringCount)), Defines::bokehEdgeBias) * modifier);
            finalColor += (getCorrectedColor(inputPixel.texCoord + (tapOffset * noise), blurDistance) * weight);
            totalWeight += weight;
        }
    }

    finalColor /= totalWeight;
    return finalColor;
}
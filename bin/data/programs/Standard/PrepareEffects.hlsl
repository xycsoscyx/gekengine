#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float calculateLuminance(float3 pixelColor)
{
    return max(dot(pixelColor, float3(0.299, 0.587, 0.114)), Math::Epsilon);
}

float3 calculateExposedColor(float3 pixelColor, float averageLuminance, float threshold, out float exposure)
{
    float KeyValue = 0.2f;

    // Use geometric mean
    averageLuminance = max(averageLuminance, 0.001f);
    float keyValue = KeyValue;
    float linearExposure = (KeyValue / averageLuminance);
    exposure = log2(max(linearExposure, Math::Epsilon));
    exposure -= threshold;
    return exp2(exposure) * pixelColor;
}

float3 calculateBrightSpots(InputPixel inputPixel, float3 pixelColor, float pixelLuminance)
{
    float averageLuminance = Resources::averageLuminance.Load(uint3(0, 0, 0));

    float exposure = 0.0;
    pixelColor = calculateExposedColor(pixelColor, averageLuminance, bloomThreshold, exposure);
    if (dot(pixelColor, 0.333) <= 0.001)
    {
        pixelColor = 0.0;
    }

    return float4(pixelColor, 1.0);
}

float calculateLuminance(InputPixel inputPixel, float pixelLuminance)
{
    return log2(max(pixelLuminance, Math::Epsilon));
}

static const float inverseAmbientOcclusionSampleCount = rcp(ambientOcclusionTapCount);

float2 getTapLocation(float tap, float randomAngle)
{
    float alpha = (tap + 0.5) * inverseAmbientOcclusionSampleCount;
    float angle = alpha * (ambientOcclusionSpiralCount * Math::Tau) + randomAngle;
    return alpha * float2(cos(angle), sin(angle));
}

float rand(float2 co)
{
    return 0.5 + (frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453)) * 0.5;
}

// http://graphics.cs.williams.edu/papers/SAOHPG12/
float calculateScalableAmbientObscurance(InputPixel inputPixel)
{
    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));

    float randomAngle = rand(inputPixel.position.xy);
    float sampleRadius = ambientOcclusionRadius / (2.0 * (surfaceDepth * Camera::maximumDistance) * Camera::fieldOfView.x);

    float totalOcclusion = 0.0;

    [unroll]
    for (int tap = 0; tap < ambientOcclusionTapCount; tap++)
    {
        float2 tapOffset = getTapLocation(tap, randomAngle);
        float2 tapCoord = inputPixel.texCoord + (tapOffset * sampleRadius);
        float tapDepth = Resources::depthBuffer.Sample(Global::pointSampler, tapCoord);
        float3 tapPosition = getViewPosition(tapCoord, tapDepth);

        float3 tapDelta = tapPosition - surfacePosition;
        float deltaMagnitude = dot(tapDelta, tapDelta);
        float deltaAngle = dot(tapDelta, surfaceNormal);
        totalOcclusion += max(0.0, deltaAngle + surfaceDepth * 0.001) / (deltaMagnitude + 0.1);
    }

    totalOcclusion *= Math::Tau * ambientOcclusionRadius * ambientOcclusionStrength / ambientOcclusionTapCount;
    totalOcclusion = min(1.0, max(0.0, 1.0 - totalOcclusion));

    // Below is directly from the Alchemy SSAO
    // Bilateral box-filter over a quad for free, respecting depth edges
    // (the difference that this makes is subtle)
    if (abs(ddx(surfacePosition.z)) < 0.02)
    {
        int pixelCoordinate = inputPixel.position.x;
        totalOcclusion -= ddx(totalOcclusion) * ((pixelCoordinate & 1) - 0.5);
    }

    if (abs(ddy(surfacePosition.z)) < 0.02)
    {
        int pixelCoordinate = inputPixel.position.y;
        totalOcclusion -= ddy(totalOcclusion) * ((pixelCoordinate & 1) - 0.5);
    }

    return totalOcclusion;
}

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float3 pixelColor = Resources::luminatedBuffer.Sample(Global::linearSampler, inputPixel.texCoord);
    float pixelLuminance = calculateLuminance(pixelColor);

    OutputPixel output;
    output.effectsBuffer.xyz = calculateBrightSpots(inputPixel, pixelColor, pixelLuminance);
    output.effectsBuffer.w = calculateScalableAmbientObscurance(inputPixel);
    output.luminanceBuffer = calculateLuminance(inputPixel, pixelLuminance);
    return output;
}

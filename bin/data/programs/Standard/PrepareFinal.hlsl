#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float inverseAmbientOcclusionSampleCount = rcp(ambientOcclusionTapCount);

float2 getTapLocation(float tap, float randomAngle)
{
    float alpha = (tap + 0.5) * inverseAmbientOcclusionSampleCount;
    float angle = alpha * (ambientOcclusionSpiralCount * Math::Tau) + randomAngle;
    return alpha * float2(cos(angle), sin(angle));
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

    OutputPixel output;
    output.ambientObscuranceBuffer = calculateScalableAmbientObscurance(inputPixel);
    output.luminanceBuffer = log2(getLuminance(pixelColor));
    return output;
}

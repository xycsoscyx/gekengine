#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

namespace Defines
{
    static const float inverseAmbientTapCount = rcp(Defines::ambientTapCount);
};

float2 getTapLocation(float tap, float randomAngle)
{
    float alpha = ((tap + 0.5) * Defines::inverseAmbientTapCount);
    float angle = ((alpha * (Defines::ambientSpiralCount * Math::Tau)) + randomAngle);
    return (alpha * float2(cos(angle), sin(angle)));
}

// Scalable Ambient Obscurance
// http://graphics.cs.williams.edu/papers/SAOHPG12/
float getShadowFactor(InputPixel inputPixel)
{
    float surfaceDepth = Resources::depthBuffer[inputPixel.screen.xy];
    float3 surfacePosition = getPositionFromSample(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer[inputPixel.screen.xy]);

    float randomAngle = random(inputPixel.screen.xy);//, Engine::worldTime);
    float sampleRadius = (Defines::ambientRadius / (2.0 * surfacePosition.z * Camera::fieldOfView.x));

    float totalOcclusion = 0.0;

    [unroll]
    for (int tap = 0; tap < Defines::ambientTapCount; tap++)
    {
        float2 tapLocation = (getTapLocation(tap, randomAngle) * sampleRadius);
        float2 tapCoord = (inputPixel.texCoord + tapLocation);

        float tapDepth = Resources::depthBuffer.SampleLevel(Global::pointSampler, tapCoord, 0);
        float3 tapPosition = getPositionFromSample(tapCoord, tapDepth);

        float3 tapDelta = (tapPosition - surfacePosition);
        float deltaMagnitude = dot(tapDelta, tapDelta);
        float deltaAngle = dot(tapDelta, surfaceNormal);
        totalOcclusion += (max(0.0, (deltaAngle + (surfaceDepth * 0.001))) / (deltaMagnitude + 0.1));
    }

    totalOcclusion *= (Math::Tau * Defines::ambientRadius * Defines::ambientStrength / Defines::ambientTapCount);
    totalOcclusion = saturate(1.0 - totalOcclusion);
    return totalOcclusion;
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return getShadowFactor(inputPixel);
}

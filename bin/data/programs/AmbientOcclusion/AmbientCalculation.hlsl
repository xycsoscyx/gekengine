#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://raw.githubusercontent.com/PeterTh/gedosato/master/pack/assets/dx9/SAO.fx
#define HighQuality                 0
#define FalloffHPG12				1
#define FalloffSmooth				2
#define FalloffMedium				3
#define FalloffQuick				4

namespace Defines
{
    static const float radiusSquared = pow(Defines::radius, 2.0);
    static const float radiusCubed = pow(Defines::radius, 3.0);
    static const float inverseTapCount = rcp(Defines::tapCount);
    static const float intensityDivR6 = Defines::intensity / pow(Defines::radius, 6.0f);
};

/** Returns a unit vector and a screen-space Defines::radius for the tap on a unit disk (the caller should scale by the actual disk Defines::radius) */
float2 getTapLocation(int tapIndex, float spinAngle, out float tapRadius)
{
    // Radius relative to tapRadius
    const float alpha = float(tapIndex + 0.5) * Defines::inverseTapCount;
    const float angle = alpha * (Defines::spiralTurns * 6.28) + spinAngle;

    tapRadius = alpha;

    float2 tapOffset;
    sincos(angle, tapOffset.y, tapOffset.x);
    return tapOffset;
}

/** Read the camera-space position of the point at screen-space pixel ssP */
float3 getPosition(float2 ssP)
{
    const float depth = Resources::depthBuffer.SampleLevel(Global::PointSampler, ssP, 0);
    return getPositionFromSample(ssP, depth);
}

/** Read the camera-space position of the point at screen-space pixel ssP + tapOffset * tapRadius.  Assumes length(tapOffset) == 1 */
float3 getOffsetPosition(float2 texCoord, float2 tapOffset, float tapRadius)
{
    const float2 tapCoord = tapRadius*tapOffset + texCoord;
    return getPosition(tapCoord);
}

/**
Compute the occlusion due to sample with index \a i about the pixel at \a texCoord that corresponds to camera-space point \a surfacePosition with unit normal \a surfaceNormal, using maximum screen-space sampling Defines::radius \a diskRadius

Note that units of H() the HPG12 paper are meters, not unitless.  The whole falloff/sampling function is therefore unitless.  this implementation, we factor out (9 / Defines::radius).

Four versions of the falloff function are implemented below
*/
float getAmbientObscurance(float2 texCoord, float3 surfacePosition, float3 surfaceNormal, float diskRadius, int tapIndex, float randomPatternRotationAngle)
{
    // Offset on the unit disk, spun for this pixel
    float tapRadius;
    const float2 tapOffset = getTapLocation(tapIndex, randomPatternRotationAngle, tapRadius);
    tapRadius *= diskRadius;

    // The occluding point camera space
    const float3 tapPosition = getOffsetPosition(texCoord, tapOffset, tapRadius);

    const float3 deltaVector = tapPosition - surfacePosition;

    const float deltaAngle = dot(deltaVector, deltaVector);
    const float normalAngle = dot(deltaVector, surfaceNormal);

    // [Boulotaur2024] yes branching is bad but choice is good...
    [branch]
    switch (Defines::falloffFunction)
    {
    case HighQuality:
        if (true)
        {
            // Addition from http://graphics.cs.williams.edu/papers/DeepGBuffer13/	
            // Epsilon inside the sqrt for rsqrt operation
            const float falloff = max(1.0 - deltaAngle * (1.0 / Defines::radiusSquared), 0.0);
            return (falloff * max((normalAngle - Defines::bias) * rsqrt(Defines::epsilon + deltaAngle), 0.0));
        }

    case FalloffHPG12:
        // A: From the HPG12 paper
        // Note large Defines::epsilon to avoid overdarkening withcracks
        return (float(deltaAngle < Defines::radiusSquared) * max((normalAngle - Defines::bias) * rcp(Defines::epsilon + deltaAngle), 0.0) * Defines::radiusSquared * 0.6);

    case FalloffSmooth:
        if (true)
        {
            // B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
            const float falloff = max(Defines::radiusSquared - deltaAngle, 0.0);
            return (pow(falloff, 3.0) * max((normalAngle - Defines::bias) * rcp(Defines::epsilon + deltaAngle), 0.0));
            // / (Defines::epsilon + deltaAngle) (optimization by BartWronski)
        }

    case FalloffMedium:
        // surfacePosition: Medium contrast (which looks better at high radii), no division.  Note that the 
        // contribution still falls off with Defines::radius^2, but we've adjusted the rate a way that is
        // more computationally efficient and happens to be aesthetically pleasing.
        return (4.0 * max(1.0 - deltaAngle * 1.0 / Defines::radiusSquared, 0.0) * max(normalAngle - Defines::bias, 0.0));

    case FalloffQuick:
        // D: Low contrast, no division operation
        return (2.0 * float(deltaAngle < Defines::radiusSquared) * max(normalAngle - Defines::bias, 0.0));

    default:
        return (4.0 * max(1.0 - deltaAngle * 1.0 / Defines::radiusSquared, 0.0) * max(normalAngle - Defines::bias, 0.0));
    };
}

// Scalable Ambient Obscurance
// http://graphics.cs.williams.edu/papers/SAOHPG12/
// https://github.com/PeterTh/gedosato/blob/master/pack/assets/dx9/SAO.fx
float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float3 surfacePosition = getPosition(inputPixel.texCoord);
    const float3 surfaceNormal = getDecodedNormal(Resources::normalBuffer.Sample(Global::PointSampler, inputPixel.texCoord));

    // McGuire noise function
    // Hash function used the HPG12 AlchemyAO paper
    const float randomPatternRotationAngle = (getNoise(inputPixel.screen.xy) * 10.0);
    const float diskRadius = (-1.0 *  Defines::radius / max(surfacePosition.z, 0.1f));
    float totalOcclusion = 0.0;

    [unroll]
    for (int tapIndex = 0; tapIndex < Defines::tapCount; ++tapIndex)
    {
        totalOcclusion += getAmbientObscurance(inputPixel.texCoord, surfacePosition, surfaceNormal, diskRadius, tapIndex, randomPatternRotationAngle);
    }

    totalOcclusion /= pow(Defines::radiusCubed, 2.0);

    if (Defines::falloffFunction == HighQuality)
    {
        // Addition from http://graphics.cs.williams.edu/papers/DeepGBuffer13/
        totalOcclusion = pow(max(0.0, 1.0 - sqrt(totalOcclusion * (3.0 / Defines::tapCount))), Defines::intensity);
    }
    else
    {
        totalOcclusion = max(0.0f, 1.0f - totalOcclusion * Defines::intensityDivR6 * (5.0f / Defines::tapCount));
    }

    // Anti-tone map to reduce contrast and drag dark region farther
    // (x^0.2 + 1.2 * x^4)/2.2
    //totalOcclusion = (pow(totalOcclusion, 0.2) + 1.2 * pow(totalOcclusion, 4.0)) / 2.2;
    return totalOcclusion;
}

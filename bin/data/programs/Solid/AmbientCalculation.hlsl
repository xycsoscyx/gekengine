#include <GEKEngine>

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://raw.githubusercontent.com/PeterTh/gedosato/master/pack/assets/dx9/SAO.fx
#define HighQuality                 0
#define FalloffHPG12				1
#define FalloffSmooth				2
#define FalloffMedium				3
#define FalloffQuick				4

namespace Options
{
    static const float RadiusSquared = pow(Options::Radius, 2.0);
    static const float RadiusCubed = pow(Options::Radius, 3.0);
    static const float InverseTapCount = rcp(Options::TapCount);
    static const float SpiralDegrees = (SpiralTurns * Math::Pi * 2.0);
    static const float IntensityModified = Options::Intensity / pow(Options::Radius, 6.0);
}; // namespace Options

/** Returns a unit vector and a screen-space Options::Radius for the tap on a unit disk (the caller should scale by the actual disk Options::Radius) */
float2 GetTapOffset(float tapIndex, float spinAngle)
{
    // Radius relative to tapRadius
    const float alpha = ((tapIndex + 0.5) * Options::InverseTapCount);
    const float angle = ((alpha * Options::SpiralDegrees) + spinAngle);

    float2 tapOffset;
    sincos(angle, tapOffset.y, tapOffset.x);
    return tapOffset * alpha;
}

/** Read the camera-space position of the point at screen-space pixel ssP */
float3 GetPositionFromDepthBuffer(float2 texCoord)
{
    uint width, height, mipMapCount;
    Resources::depthBuffer.GetDimensions(0, width, height, mipMapCount);
    const float depth = Resources::depthBuffer[texCoord * float2(width, height)];
    return GetPositionFromSampleDepth(texCoord, depth);
}

/**
Compute the occlusion due to sample with index
\a i about the pixel at
\a texCoord that corresponds to camera-space point
\a surfacePosition with unit normal
\a surfaceNormal, using maximum screen-space sampling Options::Radius
\a diskRadius

Note that units of H() the HPG12 paper are meters, not unitless.  The whole falloff/sampling function is therefore unitless.  this implementation, we factor out (9 / Options::Radius).

Four versions of the falloff function are implemented below
*/
float getAmbientObscurance(float2 texCoord, float3 surfacePosition, float3 surfaceNormal, float diskRadius, int tapIndex, float randomPatternRotationAngle)
{
    // Offset on the unit disk, spun for this pixel
    const float2 tapOffset = (GetTapOffset(tapIndex, randomPatternRotationAngle) * diskRadius);

    // The occluding point camera space
    const float3 tapPosition = GetPositionFromDepthBuffer(tapOffset + texCoord);

    const float3 deltaVector = (tapPosition - surfacePosition);

    const float deltaAngle = dot(deltaVector, deltaVector);
    const float normalAngle = dot(deltaVector, surfaceNormal);

    // [Boulotaur2024] yes branching is bad but choice is good...
    [branch]
    switch (Options::FalloffFunction)
    {
    case HighQuality:
        if (true)
        {
            // Addition from http://graphics.cs.williams.edu/papers/DeepGBuffer13/	
            // Epsilon inside the sqrt for rsqrt operation
            const float falloff = max(1.0 - deltaAngle * (1.0 / Options::RadiusSquared), 0.0);
            return (falloff * max((normalAngle - Options::Bias) * rsqrt(Options::Epsilon + deltaAngle), 0.0));
        }

    case FalloffHPG12:
        // A: From the HPG12 paper
        // Note large Options::Epsilon to avoid overdarkening withcracks
        return (float(deltaAngle < Options::RadiusSquared) * max((normalAngle - Options::Bias) * rcp(Options::Epsilon + deltaAngle), 0.0) * Options::RadiusSquared * 0.6);

    case FalloffSmooth:
        if (true)
        {
            // B: Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
            const float falloff = max(Options::RadiusSquared - deltaAngle, 0.0);
            return (pow(falloff, 3.0) * max((normalAngle - Options::Bias) * rcp(Options::Epsilon + deltaAngle), 0.0));
            // / (Options::Epsilon + deltaAngle) (optimization by BartWronski)
        }

    case FalloffMedium:
        // surfacePosition: Medium contrast (which looks better at high radii), no division.  Note that the 
        // contribution still falls off with Options::Radius^2, but we've adjusted the rate a way that is
        // more computationally efficient and happens to be aesthetically pleasing.
        return (4.0 * max(1.0 - deltaAngle * 1.0 / Options::RadiusSquared, 0.0) * max(normalAngle - Options::Bias, 0.0));

    case FalloffQuick:
        // D: Low contrast, no division operation
        return (2.0 * float(deltaAngle < Options::RadiusSquared) * max(normalAngle - Options::Bias, 0.0));

    default:
        return (4.0 * max(1.0 - deltaAngle * 1.0 / Options::RadiusSquared, 0.0) * max(normalAngle - Options::Bias, 0.0));
    };
}

// Scalable Ambient Obscurance
// http://graphics.cs.williams.edu/papers/SAOHPG12/
// https://github.com/PeterTh/gedosato/blob/master/pack/assets/dx9/SAO.fx
float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float3 surfacePosition = GetPositionFromDepthBuffer(inputPixel.texCoord);
    const float3 surfaceNormal = GetDecodedNormal(Resources::normalBuffer[inputPixel.screen.xy]);

    // McGuire noise function
    // Hash function used the HPG12 AlchemyAO paper
    const float randomPatternRotationAngle = (GetNoise(inputPixel.screen.xy) * 10.0);
    const float diskRadius = (1.0 * Options::Radius / max(surfacePosition.z, 0.1f));
    float totalOcclusion = 0.0;

    [unroll]
    for (int tapIndex = 0; tapIndex < Options::TapCount; ++tapIndex)
    {
        totalOcclusion += getAmbientObscurance(inputPixel.texCoord, surfacePosition, surfaceNormal, diskRadius, tapIndex, randomPatternRotationAngle);
    }

    totalOcclusion /= pow(Options::RadiusCubed, 2.0);

    if (Options::FalloffFunction == HighQuality)
    {
        // Addition from http://graphics.cs.williams.edu/papers/DeepGBuffer13/
        totalOcclusion = pow(max(0.0, 1.0 - sqrt(totalOcclusion * (3.0 / Options::TapCount))), Options::Intensity);
    }
    else
    {
        totalOcclusion = max(0.0f, 1.0f - totalOcclusion * Options::IntensityModified * (5.0f / Options::TapCount));
    }

    // Anti-tone map to reduce contrast and drag dark region farther
    // (x^0.2 + 1.2 * x^4)/2.2
    //totalOcclusion = (pow(totalOcclusion, 0.2) + 1.2 * pow(totalOcclusion, 4.0)) / 2.2;
    return totalOcclusion;
}

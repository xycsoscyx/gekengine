#include <GEKEngine>

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

namespace Options
{
    namespace AmbientOcclusion
    {
        static const float GaussianSigmaSquared = square(Options::AmbientOcclusion::GaussianSigma);
    }; // namespace AmbientOcclusion
}; // namespace Options

float getGaussianWeight(float offset)
{
    static const float numerator = (1.0 / (sqrt(Math::Tau) * Options::AmbientOcclusion::GaussianSigma));
    static const float denominator = (1.0 / (2.0 * Options::AmbientOcclusion::GaussianSigmaSquared));
    return (numerator * exp(-(offset * offset) * denominator));
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float surfaceDepth = GetLinearDepthFromSampleDepth(Resources::depthBuffer[inputPixel.screen.xy]);
    float totalOcclusion = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int tapIndex = -Options::AmbientOcclusion::GaussianRadius; tapIndex <= Options::AmbientOcclusion::GaussianRadius; ++tapIndex)
    {
        const int2 tapCoord = (inputPixel.screen.xy + (Options::AmbientOcclusion::BlurAxis * tapIndex));
        const float tapDepth = GetLinearDepthFromSampleDepth(Resources::depthBuffer[tapCoord]);
        const float tapOcclusion = Resources::inputBuffer[tapCoord];

        // spatial domain: offset gaussian tap
        float tapWeight = getGaussianWeight(abs(tapIndex));

        // range domain (the "bilateral" tapWeight). As depth difference increases, decrease tapWeight.
        tapWeight *= max(0.0, 1.0 - (Camera::FarClip * Options::AmbientOcclusion::EdgeSharpness) * abs(tapDepth - surfaceDepth));

        totalOcclusion += tapOcclusion * tapWeight;
        totalWeight += tapWeight;
    }

    return totalOcclusion / (totalWeight + Math::Epsilon);
}

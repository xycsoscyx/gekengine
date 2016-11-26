#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float getGaussianWeight(float offset)
{
    static const float numerator = (1.0 / (sqrt(Math::Tau) * Defines::gaussianSigma));
    static const float denominator = (1.0 / (2.0 * Defines::gaussianSigma*Defines::gaussianSigma));
    return (numerator * exp(-(offset * offset) * denominator));
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float surfaceDepth = getLinearDepthFromSample(Resources::depthBuffer[inputPixel.screen.xy]);
    float totalOcclusion = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int tapIndex = -Defines::gaussianRadius; tapIndex <= Defines::gaussianRadius; ++tapIndex)
    {
        const int2 tapCoord = (inputPixel.screen.xy + (Defines::blurAxis * tapIndex));
        const float tapDepth = getLinearDepthFromSample(Resources::depthBuffer[tapCoord]);
        const float tapOcclusion = Resources::inputBuffer[tapCoord];

        // spatial domain: offset gaussian tap
        float tapWeight = getGaussianWeight(abs(tapIndex));

        // range domain (the "bilateral" tapWeight). As depth difference increases, decrease tapWeight.
        tapWeight *= max(0.0, 1.0 - (Camera::farClip * Defines::edgeSharpness) * abs(tapDepth - surfaceDepth));

        totalOcclusion += tapOcclusion * tapWeight;
        totalWeight += tapWeight;
    }

    return totalOcclusion / (totalWeight + Math::Epsilon);
}

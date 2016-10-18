#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float calculateGaussianWeight(float offset)
{
    static const float gaussian = (1.0f / (sqrt(Math::Tau) * Defines::gaussianSigma));
    static const float denominator = rcp(2.0 * pow(Defines::gaussianSigma, 2.0));
    return ((gaussian * exp(-pow(offset, 2.0) * denominator)) / 2.0);
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float surfaceDepth = getLinearDepthFromSample(Resources::depthBuffer[inputPixel.screen.xy]);
    float totalOcclusion = Resources::inputBuffer[inputPixel.screen.xy];

    float totalWeight = calculateGaussianWeight(0);
    totalOcclusion *= totalWeight;

    [unroll]
    for (int tapIndex = -Defines::gaussianRadius; tapIndex <= Defines::gaussianRadius; ++tapIndex)
    {
        // We already handled the zero case above.  This loop should be unrolled and the branch discarded
        [branch]
        if (tapIndex != 0)
        {
            float2 tapCoord = inputPixel.screen.xy + Defines::blurAxis * tapIndex;
            float tapDepth = getLinearDepthFromSample(Resources::depthBuffer[tapCoord]);
            float tapOcclusion = Resources::inputBuffer[tapCoord];

            // spatial domain: offset gaussian tap
            float tapWeight = 0.3 + calculateGaussianWeight(abs(tapIndex));

            // range domain (the "bilateral" tapWeight). As depth difference increases, decrease tapWeight.
            tapWeight *= rcp(Math::Epsilon + Defines::edgeSharpness * abs(tapDepth - surfaceDepth));
            //tapWeight *= max(0.0, 1.0 - (800.0 * Defines::edgeSharpness) * abs(tapDepth - surfaceDepth));

            totalOcclusion += tapOcclusion * tapWeight;
            totalWeight += tapWeight;
        }
    }

    return totalOcclusion / (totalWeight + Math::Epsilon);
}

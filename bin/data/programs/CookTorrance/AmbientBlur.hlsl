#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float calculateGaussianWeight(float offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * Defines::gaussianSigma));
    static const float d = rcp(2.0 * square(Defines::gaussianSigma));
    return (g * exp(-square(offset) * d)) / 2.0;
}

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float surfaceDepth = getLinearDepthFromSample(Resources::depth[inputPixel.screen.xy]);

    float finalValue = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -Defines::gaussianRadius; offset <= Defines::gaussianRadius; offset++)
    {
        int2 sampleOffset = (offset * int2(Defines::blurAxis));
        int2 sampleCoord = (inputPixel.screen.xy + sampleOffset);
        float sampleDepth = getLinearDepthFromSample(Resources::depth[sampleCoord]);
        float depthDelta = abs(surfaceDepth - sampleDepth);

        float sampleWeight = 1.0;//calculateGaussianWeight(offset) * rcp(Math::Epsilon + Defines::bilateralEdgeSharpness * depthDelta);

        float sampleValue = Resources::sourceBuffer[sampleCoord];
        finalValue += (sampleValue * sampleWeight);
        totalWeight += sampleWeight;
    }

    return (finalValue * rcp(totalWeight + Math::Epsilon));
}

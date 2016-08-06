#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float calculateGaussianWeight(float offset)
{
    static const float g = (1.0f / (sqrt(Math::Tau) * Defines::gaussianSigma));
    static const float d = rcp(2.0 * square(Defines::gaussianSigma));
    return (g * exp(-square(offset) * d)) / 2;
}

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float2 pixelSize = rcp(Shader::targetSize);
    float surfaceDepth = Resources::depth[inputPixel.screen.xy];

    float finalValue = 0.0;
    float totalWeight = 0.0;

    [unroll]
    for (int offset = -Defines::guassianRadius; offset <= Defines::guassianRadius; offset++)
    {
        int2 sampleOffset = (offset * Defines::blurAxis);
        float sampleDepth = Resources::depth[inputPixel.screen.xy + sampleOffset];
        float depthDelta = abs(surfaceDepth - sampleDepth);

        float sampleWeight = calculateGaussianWeight(offset) * rcp(Math::Epsilon + Defines::bilateralEdgeSharpness * depthDelta);

        float sampleValue = Resources::sourceBuffer[inputPixel.screen.xy + sampleOffset];
        finalValue += (sampleValue * sampleWeight);
        totalWeight += sampleWeight;
    }

    OutputPixel outputPixel;
    outputPixel.ambientBuffer = (finalValue * rcp(totalWeight + Math::Epsilon));
    return outputPixel;
}

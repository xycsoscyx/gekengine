#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float2 computeRefractionOffset(float linearDepth, float3 normal, float3 position, float eta)
{
    return 0.0;
}

/* Diffusion scaling constant. Adjust based on the precision of the diffusion texture channel. */
static const float k_0 = 8.0;

/** Focus rate. Increase to make objects come into focus behind a frosted glass surface more quickly, decrease to defocus them quickly. */
static const float k_1 = 0.1;

// http://graphics.cs.williams.edu/papers/TransparencyI3D16/McGuire2016Transparency.pdf
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    float3 premultipliedReflectionAndEmission = albedo.rgb;
    float coverage = albedo.a;
    float3 transmissionCoefficient = 1.0 - albedo.a;
    float collimation = saturate(albedo.a * 0.25 + 0.8);
    float etaRatio = 0.0;

    OutputPixel outputPixel;

    outputPixel.modulationDiffusionBuffer.rgb = coverage * (1.0 - transmissionCoefficient);

    coverage *= 1.0 - (transmissionCoefficient.r + transmissionCoefficient.g + transmissionCoefficient.b) * (1.0 / 3.0);
    float adjustedDepth = 1.0 - inputPixel.screen.z * 0.99;
    float weight = clamp(coverage * adjustedDepth * adjustedDepth * adjustedDepth * 1e3, 1e-2, 3e2 * 0.1);
    outputPixel.accumulationBuffer = float4(premultipliedReflectionAndEmission, coverage) * weight;

    float transparentDepth = inputPixel.position.z - 4.0;
    float2 refractionOffset = (etaRatio == 1.0) ? 0.0 : computeRefractionOffset(transparentDepth, inputPixel.normal, inputPixel.position, etaRatio);
    float linearDepth = getLinearDepthFromSample(Resources::depthCopy[inputPixel.screen.xy]);
    outputPixel.modulationDiffusionBuffer.a = k_0 * coverage * (1.0 - collimation) * (1.0 - k_1 / (k_1 + inputPixel.position.z - linearDepth)) / abs(inputPixel.position.z);
    outputPixel.modulationDiffusionBuffer.a *= outputPixel.modulationDiffusionBuffer.a;

    [branch]
    if (outputPixel.modulationDiffusionBuffer.a > 0.0)
    {
        outputPixel.modulationDiffusionBuffer.a = max(outputPixel.modulationDiffusionBuffer.a, 1.0 / 256.0);
    }

    outputPixel.refractionBuffer = refractionOffset * coverage * 8.0;

    return outputPixel;
}

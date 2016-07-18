#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float2 computeRefractionOffset(float viewDepth, float3 normal, float3 position, float eta)
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
    float collimation = albedo.a * 0.25 + 0.75;
    float etaRatio = 0.0;
    float3 csPosition = inputPixel.viewPosition;
    float3 csNormal = inputPixel.viewNormal;

    OutputPixel outputPixel;

    outputPixel.modulationDiffusionBuffer.rgb = coverage * (1.0 - transmissionCoefficient);

    coverage *= 1.0 - (transmissionCoefficient.r + transmissionCoefficient.g + transmissionCoefficient.b) * (1.0 / 3.0);
    float tmp = 1.0 - inputPixel.position.z * 0.99;
    float w = clamp(coverage * tmp * tmp * tmp * 1e3, 1e-2, 3e2 * 0.1);
    outputPixel.accumulationBuffer = float4(premultipliedReflectionAndEmission, coverage) * w;

    float backgroundZ = csPosition.z - 4.0;
    float2 refractionOffset = (etaRatio == 1.0) ? 0.0 : computeRefractionOffset(backgroundZ, csNormal, csPosition, etaRatio);
    float trueBackgroundCSZ = getViewDepthFromProjectedDepth(Resources::depthBuffer[inputPixel.position.xy]);
    outputPixel.modulationDiffusionBuffer.a = k_0 * coverage * (1.0 - collimation) * (1.0 - k_1 / (k_1 + csPosition.z - trueBackgroundCSZ)) / abs(csPosition.z);
    outputPixel.modulationDiffusionBuffer.a *= outputPixel.modulationDiffusionBuffer.a;

    [branch]
    if (outputPixel.modulationDiffusionBuffer.a > 0.0)
    {
        outputPixel.modulationDiffusionBuffer.a = max(outputPixel.modulationDiffusionBuffer.a, 1.0 / 256.0);
    }

    outputPixel.refractionBuffer = refractionOffset * coverage * 8.0;

    return outputPixel;
}

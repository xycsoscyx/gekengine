#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float2 getRefractOffset(float sceneDepth, float3 normal, float3 position, float eta)
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

    float3 premultipliedReflectionAndEmission = 0.0;
    float3 transmissionCoefficient = albedo.rgb;
    float coverage = albedo.a;
    float collimation = albedo.a;
    float etaRatio = 0.0;

    OutputPixel outputPixel;

    outputPixel.modulationDiffusionBuffer.rgb = coverage * (1.0 - transmissionCoefficient);

    coverage *= 1.0 - (transmissionCoefficient.r + transmissionCoefficient.g + transmissionCoefficient.b) * (1.0 / 3.0);
    float adjustedDepth = 1.0 - inputPixel.position.z * 0.99;
    float weight = clamp(coverage * cube(adjustedDepth) * 1e3, 1e-2, 3e2 * 0.1);
    outputPixel.accumulationBuffer = float4(premultipliedReflectionAndEmission, coverage) * weight;

    float backgroundDepth = inputPixel.position.z - 4.0;
    float sceneDepth = getSceneDepth(Resources::depthBuffer[inputPixel.position.xy * Shader::pixelSize]);
    outputPixel.modulationDiffusionBuffer.a = square(k_0 * coverage * (1.0 - collimation) * (1.0 - k_1 / (k_1 + inputPixel.viewPosition.z - sceneDepth)) / abs(inputPixel.viewPosition.z));
    if (outputPixel.modulationDiffusionBuffer.a > 0.0)
    {
        outputPixel.modulationDiffusionBuffer.a = max(outputPixel.modulationDiffusionBuffer.a, 1.0 / 256.0);
    }

    float2 refractionOffset = (etaRatio == 1.0) ? 0.0 : getRefractOffset(backgroundDepth, inputPixel.viewNormal, inputPixel.viewPosition, etaRatio);
    outputPixel.refractionBuffer = refractionOffset * coverage * 8.0;

    return outputPixel;
}

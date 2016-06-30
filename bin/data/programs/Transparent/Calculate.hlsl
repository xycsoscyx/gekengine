#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const float collimation = 1.0;
static const float k_0 = 120.0 / 63.0;
static const float k_1 = 0.05;

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float3 transmission = inputPixel.color1.rgb;
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    float3 premultipliedColor = (albedo.rgb * albedo.a);
    float coverage = albedo.a;
    float sceneDepth = getLinearDepth(Resources::depthBuffer[inputPixel.position.xy]);

    float netCoverage = coverage * (1.0 - dot(transmission, (1.0 / 3.0)));
    float tmp = (1.0 - inputPixel.position.z * 0.99) * netCoverage * 10.0;
    float w = clamp(tmp * tmp * tmp, 0.01, 30.0);

    float4 alpha = float4(premultipliedColor * coverage, netCoverage) * w;
    float3 beta = coverage * (1.0 - transmission) * (1.0 / 3.0);
    float diffusion = k_0 * netCoverage * (1.0 - collimation) * (1.0 - k_1 / (k_1 + inputPixel.viewPosition.z - sceneDepth)) / abs(inputPixel.viewPosition.z);
    diffusion = max((diffusion * diffusion), 1.0 / 256.0);

    OutputPixel outputPixel;
    outputPixel.alphaBuffer = alpha;
    outputPixel.betaBuffer.rgb = beta;
    outputPixel.betaBuffer.a = diffusion;
    return outputPixel;
}

#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

namespace Settings
{
    static const uint Equation = 9;
};

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    float reveal = albedo.a;
    float weight = albedo.a;

    const float3 transmission = albedo.rgb;
    weight *= saturate(1.0 - dot(transmission, (1.0 / 3.0)));

    // Soften edges when transparent surfaces intersect solid surfaces
    float linearDepth = getLinearDepthFromSample(Resources::depthBuffer[inputPixel.screen.xy]);
    float depthDelta = saturate((linearDepth - inputPixel.position.z) * 2.5);
    weight *= depthDelta;
    
    [branch]
    switch (Settings::Equation)
    {
    case 7: // View Depth
        weight *= (10.0 / (Math::Epsilon + pow((inputPixel.position.z / 5.0), 2.0) + pow((inputPixel.position.z / 200.0), 6.0)));
        break;

    case 8: // View Depth
        weight *= (10.0 / (Math::Epsilon + pow((inputPixel.position.z / 10.0), 3.0) + pow((inputPixel.position.z / 200.0), 6.0)));
        break;

    case 9: // View Depth
        weight *= (0.03 / (Math::Epsilon + pow((inputPixel.position.z / 200.0), 4.0)));
        break;

    case 10: // Clip Depth
        weight *= dot(3e3, pow((1.0 - inputPixel.screen.z), 3.0));
        break;
    };

    weight = clamp(weight, 1e-2, 3e3);

    OutputPixel outputPixel;
    outputPixel.accumulationBuffer = float4((albedo.rgb * weight), weight);
    outputPixel.revealBuffer = reveal;
    return outputPixel;
}

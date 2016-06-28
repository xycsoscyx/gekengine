#include "GEKEngine"

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
    albedo.rgb *= albedo.a;

    const float3 transmission = inputPixel.color1.rgb;
    //albedo.a *= saturate(1.0 - dot(inputPixel.color1.rgb, (1.0 / 3.0)));

    // Soften edges when transparent surfaces intersect solid surfaces
    const float sceneDepth = getLinearDepth(Resources::depthBuffer[inputPixel.position.xy]);
    const float depthDelta = saturate((sceneDepth - inputPixel.viewPosition.z) * 2.5);
    //albedo.a *= depthDelta;

    float reveal;
    switch (Settings::Equation)
    {
    case 7: // View Depth
        reveal = rcp(Math::Epsilon + pow((inputPixel.viewPosition.z / 5.0), 2.0) + pow((inputPixel.viewPosition.z / 200.0), 6.0));
        break;

    case 8: // View Depth
        reveal = rcp(Math::Epsilon + pow((inputPixel.viewPosition.z / 10.0), 3.0) + pow((inputPixel.viewPosition.z / 200.0), 6.0));
        break;

    case 9: // View Depth
        reveal = (0.03 / (Math::Epsilon + pow((inputPixel.viewPosition.z / 200.0), 4.0)));
        break;

    case 10: // Clip Depth
        reveal = (30000 * pow((1.0 - inputPixel.position.z), 3.0));
        break;
    };

    OutputPixel outputPixel;
    outputPixel.accumulationBuffer = (albedo * reveal);
    outputPixel.revealBuffer = albedo.a;
    return outputPixel;
}

#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

namespace Settings
{
    static const uint Equation = 10;
};

// Weight Based OIT
// http://jcgt.org/published/0002/02/09/paper.pdf
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    const float clipDepth = inputPixel.position.z;
    const float viewDepth = inputPixel.viewPosition.z;
    const float4 color = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    float coverage = color.a;

    // Need to store transmission in a separate color?
    coverage *= (1.0 - (color.r + color.g + color.b) * (1.0 / 3.0));

    // Soften edges when transparent surfaces intersect solid surfaces
    float sceneDepth = getLinearDepth(Resources::depthBuffer[inputPixel.position.xy]);
    float depthDelta = saturate((sceneDepth - viewDepth) * 2.5);
    coverage *= depthDelta;

    float reveal;
    switch (Settings::Equation)
    {
    case 7:
        reveal = (10.0 / (10e-5 + pow((viewDepth / 5.0), 2.0) + pow((viewDepth / 200.0), 6.0)));
        break;

    case 8:
        reveal = (10.0 / (10e-5 + pow((viewDepth / 10.0), 3.0) + pow((viewDepth / 200.0), 6.0)));
        break;

    case 9:
        reveal = (0.03 / (10e-5 + pow((viewDepth / 200.0), 4.0)));
        break;

    case 10:
        reveal = ((3 * 10e3) * pow((1.0 - clipDepth), 3.0));
        break;

    default:
        reveal = 0.0;
        break;
    };

    reveal = clamp(reveal, Math::Epsilon, 3 * 10e3);

    OutputPixel outputPixel;
    outputPixel.accumulationBuffer = (float4((color.rgb * color.a), coverage) * reveal);
    outputPixel.revealBuffer = (coverage * reveal);
    return outputPixel;
}

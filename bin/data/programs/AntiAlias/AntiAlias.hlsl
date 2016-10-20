#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://github.com/mattdesl/glsl-fxaa
//optimized version for mobile, where dependent 
//texture reads can be a bottleneck
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float3 colorMD = Resources::screenBuffer[inputPixel.screen.xy];
    const float3 colorNW = Resources::screenBuffer[inputPixel.screen.xy + float2(-1, +1)];
    const float3 colorNE = Resources::screenBuffer[inputPixel.screen.xy + float2(+1, +1)];
    const float3 colorSW = Resources::screenBuffer[inputPixel.screen.xy + float2(-1, -1)];
    const float3 colorSE = Resources::screenBuffer[inputPixel.screen.xy + float2(+1, -1)];

    const float luminanceMD = getLuminance(colorMD);
    const float luminanceNW = getLuminance(colorNW);
    const float luminanceNE = getLuminance(colorNE);
    const float luminanceSW = getLuminance(colorSW);
    const float luminanceSE = getLuminance(colorSE);
    const float luminanceMinimum = min(luminanceMD, min(min(luminanceNW, luminanceNE), min(luminanceSW, luminanceSE)));
    const float luminanceMaximum = max(luminanceMD, max(max(luminanceNW, luminanceNE), max(luminanceSW, luminanceSE)));

    float2 direection;
    direection.x = -((luminanceNW + luminanceNE) - (luminanceSW + luminanceSE));
    direection.y = ((luminanceNW + luminanceSW) - (luminanceNE + luminanceSE));
    const float dirReduce = max((luminanceNW + luminanceNE + luminanceSW + luminanceSE) * (0.25 * Defines::reduceMultiplier), Defines::reduceMinimum);

    const float recipricalDirection = 1.0 / (min(abs(direection.x), abs(direection.y)) + dirReduce);
    direection = min(Defines::spanMaximum, max(-Defines::spanMaximum, direection * recipricalDirection)) * Shader::pixelSize;

    float3 colorA = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + direection * (1.0 / 3.0 - 0.5), 0);
    colorA += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + direection * (2.0 / 3.0 - 0.5), 0);
    colorA *= 0.5;

    float3 colorB = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + direection * -0.5, 0);
    colorB += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + direection * 0.5, 0);
    colorB = ((colorA * 0.5) + (0.25 * colorB));

    const float luminanceB = getLuminance(colorB);
    if ((luminanceB < luminanceMinimum) || (luminanceB > luminanceMaximum))
    {
        return colorA;
    }
    else
    {
        return colorB;
    }
}

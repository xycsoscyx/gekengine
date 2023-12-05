#include <GEKEngine>

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://github.com/mattdesl/glsl-fxaa
//optimized version for mobile, where dependent 
//texture reads can be a bottleneck
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float luminanceMD = GetLuminance(Resources::inputBuffer[inputPixel.screen.xy].xyz);
    const float luminanceNW = GetLuminance(Resources::inputBuffer[inputPixel.screen.xy + float2(-1, -1)].xyz);
    const float luminanceNE = GetLuminance(Resources::inputBuffer[inputPixel.screen.xy + float2(+1, -1)].xyz);
    const float luminanceSW = GetLuminance(Resources::inputBuffer[inputPixel.screen.xy + float2(-1, +1)].xyz);
    const float luminanceSE = GetLuminance(Resources::inputBuffer[inputPixel.screen.xy + float2(+1, +1)].xyz);

    const float luminanceMinimum = min(luminanceMD, min(min(luminanceNW, luminanceNE), min(luminanceSW, luminanceSE)));
    const float luminanceMaximum = max(luminanceMD, max(max(luminanceNW, luminanceNE), max(luminanceSW, luminanceSE)));

    float2 direction;
    direction.x = -((luminanceNW + luminanceNE) - (luminanceSW + luminanceSE));
    direction.y =  ((luminanceNW + luminanceSW) - (luminanceNE + luminanceSE));
    const float directionReduces = max((luminanceNW + luminanceNE + luminanceSW + luminanceSE) * (0.25 * Options::ReduceMultiplier), Options::ReduceMinimum);

    const float reciprocalDirection = 1.0 / (min(abs(direction.x), abs(direction.y)) + directionReduces);
    direction = min(Options::SpanMaximum.xx, max(-Options::SpanMaximum.xx, direction * reciprocalDirection));

    float3 colorA = 0.5 * (Resources::inputBuffer[inputPixel.screen.xy + direction * (1.0 / 3.0 - 0.5)].xyz
                        +  Resources::inputBuffer[inputPixel.screen.xy + direction * (2.0 / 3.0 - 0.5)].xyz);

    float3 colorB = colorA * (1.0 / 2.0) + (1.0 / 4.0)
                  * (Resources::inputBuffer[inputPixel.screen.xy + direction * -0.5].xyz
                  +  Resources::inputBuffer[inputPixel.screen.xy + direction *  0.5].xyz);

    const float luminanceB = GetLuminance(colorB);
    if ((luminanceB < luminanceMinimum) || (luminanceB > luminanceMaximum))
    {
        return colorA;
    }
    else
    {
        return colorB;
    }
}
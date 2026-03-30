#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

// https://github.com/mattdesl/glsl-fxaa
//optimized version for mobile, where dependent 
//texture reads can be a bottleneck
[shader("fragment")]
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    uint textureWidth = 0;
    uint textureHeight = 0;
    Resources::inputBuffer.GetDimensions(textureWidth, textureHeight);

    const int2 minCoord = int2(0, 0);
    const int2 maxCoord = int2(int(max(textureWidth, 1u)) - 1, int(max(textureHeight, 1u)) - 1);
    const int2 screenCoord = clamp(int2(inputPixel.screen.xy), minCoord, maxCoord);
    const int2 screenNW = clamp(screenCoord + int2(-1, -1), minCoord, maxCoord);
    const int2 screenNE = clamp(screenCoord + int2(+1, -1), minCoord, maxCoord);
    const int2 screenSW = clamp(screenCoord + int2(-1, +1), minCoord, maxCoord);
    const int2 screenSE = clamp(screenCoord + int2(+1, +1), minCoord, maxCoord);

    const float luminanceMD = GetLuminance(Resources::inputBuffer[uint2(screenCoord)].xyz);
    const float luminanceNW = GetLuminance(Resources::inputBuffer[uint2(screenNW)].xyz);
    const float luminanceNE = GetLuminance(Resources::inputBuffer[uint2(screenNE)].xyz);
    const float luminanceSW = GetLuminance(Resources::inputBuffer[uint2(screenSW)].xyz);
    const float luminanceSE = GetLuminance(Resources::inputBuffer[uint2(screenSE)].xyz);

    const float luminanceMinimum = min(luminanceMD, min(min(luminanceNW, luminanceNE), min(luminanceSW, luminanceSE)));
    const float luminanceMaximum = max(luminanceMD, max(max(luminanceNW, luminanceNE), max(luminanceSW, luminanceSE)));

    float2 direction;
    direction.x = -((luminanceNW + luminanceNE) - (luminanceSW + luminanceSE));
    direction.y =  ((luminanceNW + luminanceSW) - (luminanceNE + luminanceSE));
    const float directionReduces = max((luminanceNW + luminanceNE + luminanceSW + luminanceSE) * (0.25 * Options::ReduceMultiplier), Options::ReduceMinimum);

    const float reciprocalDirection = 1.0 / (min(abs(direction.x), abs(direction.y)) + directionReduces);
    direction = min(Options::SpanMaximum.xx, max(-Options::SpanMaximum.xx, direction * reciprocalDirection));

    const int2 screenA0 = clamp(int2(inputPixel.screen.xy + direction * (1.0 / 3.0 - 0.5)), minCoord, maxCoord);
    const int2 screenA1 = clamp(int2(inputPixel.screen.xy + direction * (2.0 / 3.0 - 0.5)), minCoord, maxCoord);
    float3 colorA = 0.5 * (Resources::inputBuffer[uint2(screenA0)].xyz
                        +  Resources::inputBuffer[uint2(screenA1)].xyz);

    const int2 screenB0 = clamp(int2(inputPixel.screen.xy + direction * -0.5), minCoord, maxCoord);
    const int2 screenB1 = clamp(int2(inputPixel.screen.xy + direction *  0.5), minCoord, maxCoord);
    float3 colorB = colorA * (1.0 / 2.0) + (1.0 / 4.0)
                  * (Resources::inputBuffer[uint2(screenB0)].xyz
                  +  Resources::inputBuffer[uint2(screenB1)].xyz);

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
#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://github.com/mattdesl/glsl-fxaa
//optimized version for mobile, where dependent 
//texture reads can be a bottleneck
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    static const float2 v_rgbNW = float2(-1.0, +1.0);
    static const float2 v_rgbNE = float2(+1.0, +1.0);
    static const float2 v_rgbSW = float2(-1.0, -1.0);
    static const float2 v_rgbSE = float2(+1.0, -1.0);
    static const float2 v_rgbM  = float2(+0.0, +0.0);

    const float3 rgbNW = Resources::screenBuffer[inputPixel.screen.xy + v_rgbNW];
    const float3 rgbNE = Resources::screenBuffer[inputPixel.screen.xy + v_rgbNE];
    const float3 rgbSW = Resources::screenBuffer[inputPixel.screen.xy + v_rgbSW];
    const float3 rgbSE = Resources::screenBuffer[inputPixel.screen.xy + v_rgbSE];
    const float3 texColor = Resources::screenBuffer[inputPixel.screen.xy + v_rgbM];
    const float3 rgbM = texColor.xyz;

    const float lumaNW = getLuminance(rgbNW);
    const float lumaNE = getLuminance(rgbNE);
    const float lumaSW = getLuminance(rgbSW);
    const float lumaSE = getLuminance(rgbSE);
    const float lumaM = getLuminance(rgbM);
    const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    const float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * Defines::reduceMultiplier), Defines::reduceMinimum);

    const float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(Defines::spanMaximum, max(-Defines::spanMaximum, dir * rcpDirMin)) * Shader::pixelSize;

    float3 rgbA = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * (1.0 / 3.0 - 0.5), 0);
    rgbA += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * (2.0 / 3.0 - 0.5), 0);
    rgbA *= 0.5;

    float3 rgbB = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * -0.5, 0);
    rgbB += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * 0.5, 0);
    rgbB = ((rgbA * 0.5) + (0.25 * rgbB));

    float3 color;
    const float lumaB = getLuminance(rgbB);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        color = rgbA;
    }
    else
    {
        color = rgbB;
    }

    return color;
}

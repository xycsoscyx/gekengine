#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

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

    float3 color;
    float3 rgbNW = Resources::screenBuffer[inputPixel.screen.xy + v_rgbNW];
    float3 rgbNE = Resources::screenBuffer[inputPixel.screen.xy + v_rgbNE];
    float3 rgbSW = Resources::screenBuffer[inputPixel.screen.xy + v_rgbSW];
    float3 rgbSE = Resources::screenBuffer[inputPixel.screen.xy + v_rgbSE];
    float3 texColor = Resources::screenBuffer[inputPixel.screen.xy + v_rgbM];
    float3 rgbM = texColor.xyz;

    float lumaNW = getLuminance(rgbNW);
    float lumaNE = getLuminance(rgbNE);
    float lumaSW = getLuminance(rgbSW);
    float lumaSE = getLuminance(rgbSE);
    float lumaM = getLuminance(rgbM);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * Defines::reduceMultiplier), Defines::reduceMinimum);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(Defines::spanMaximum, max(-Defines::spanMaximum, dir * rcpDirMin)) * Shader::pixelSize;

    float3 rgbA = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * (1.0 / 3.0 - 0.5), 0);
    rgbA += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * (2.0 / 3.0 - 0.5), 0);
    rgbA *= 0.5;

    float3 rgbB = Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * -0.5, 0);
    rgbB += Resources::screenBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord + dir * 0.5, 0);
    rgbB = ((rgbA * 0.5) + (0.25 * rgbB));

    float lumaB = getLuminance(rgbB);
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

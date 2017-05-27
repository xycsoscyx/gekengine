#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

// from http://www.java-gaming.org/index.php?topic=35123.0
float4 GetCubicFactors(float v)
{
    float4 n = float4(1.0, 2.0, 3.0, 4.0) - v;
    float4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return float4(x, y, z, w) * (1.0 / 6.0);
}

float3 GetBiCubicSample(in float2 screenCoord, float glassLevel)
{
    float mipMapLevel = floor(glassLevel);
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    screenCoord -= 0.5;

    float2 screenCoordFraction = frac(screenCoord);
    screenCoord -= screenCoordFraction;

    float4 xCubicFactor = GetCubicFactors(screenCoordFraction.x);
    float4 yCubicFactor = GetCubicFactors(screenCoordFraction.y);

    float4 centerCoord = screenCoord.xxyy + float2(-0.5, +1.5).xyxy;

    float4 scaleFactor = float4(xCubicFactor.xz + xCubicFactor.yw, yCubicFactor.xz + yCubicFactor.yw);
    float4 texCoord = centerCoord + float4(xCubicFactor.yw, yCubicFactor.yw) / scaleFactor;
    texCoord *= mipMapScale * Shader::TargetPixelSize.xxyy;

    float3 texSample0 = Resources::glassBuffer.SampleLevel(Global::TextureSampler, texCoord.xz, glassLevel);
    float3 texSample1 = Resources::glassBuffer.SampleLevel(Global::TextureSampler, texCoord.yz, glassLevel);
    float3 texSample2 = Resources::glassBuffer.SampleLevel(Global::TextureSampler, texCoord.xw, glassLevel);
    float3 texSample3 = Resources::glassBuffer.SampleLevel(Global::TextureSampler, texCoord.yw, glassLevel);

    float scaleFactorX = scaleFactor.x / (scaleFactor.x + scaleFactor.y);
    float scaleFactorY = scaleFactor.z / (scaleFactor.z + scaleFactor.w);

    return lerp(
        lerp(texSample3, texSample2, scaleFactorX),
        lerp(texSample1, texSample0, scaleFactorX), scaleFactorY);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord.xy);

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.screenCoord);

    float3 surfaceNormal;
    // assume normals are stored as 3Dc format, so generate the Z value
    surfaceNormal.xy = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord.xy).xy;
    surfaceNormal.xy = ((surfaceNormal.xy * 2.0) - 1.0);
    surfaceNormal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    surfaceNormal.z = sqrt(1.0 - dot(surfaceNormal.xy, surfaceNormal.xy));
    surfaceNormal = mul(surfaceNormal, viewBasis);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, 0.25, 0.75);

    float glassLevel = (materialRoughness * 5.0);
    float3 glassColor = GetBiCubicSample(inputPixel.screen.xy, glassLevel);
    return (surfaceIrradiance + (glassColor * materialAlbedo));
}

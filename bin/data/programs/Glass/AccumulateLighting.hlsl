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

// https://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl
float3 GetBiCubicSample(in float2 screenCoord, float glassLevel)
{
    float mipMapLevel = floor(glassLevel * 5.0);
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    float fx = frac(screenCoord.x);
    float fy = frac(screenCoord.y);
    screenCoord.x -= fx;
    screenCoord.y -= fy;

    float4 xcubic = GetCubicFactors(fx);
    float4 ycubic = GetCubicFactors(fy);

    float4 c = screenCoord.xxyy + float2(-0.5, 1.5).xyxy;
    float4 s = float4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    float4 offset = c + float4(xcubic.yw, ycubic.yw) / s;
    offset *= Shader::TargetPixelSize.xxyy * mipMapScale;

    float3 sample0 = Resources::glassBuffer.SampleLevel(Global::MipMapSampler, offset.xz, mipMapLevel);
    float3 sample1 = Resources::glassBuffer.SampleLevel(Global::MipMapSampler, offset.yz, mipMapLevel);
    float3 sample2 = Resources::glassBuffer.SampleLevel(Global::MipMapSampler, offset.xw, mipMapLevel);
    float3 sample3 = Resources::glassBuffer.SampleLevel(Global::MipMapSampler, offset.yw, mipMapLevel);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return lerp(
        lerp(sample3, sample2, sx),
        lerp(sample1, sample0, sx), sy);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord.xy);

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = GetCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.screenCoord);

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
    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);

    float materialThickness = Resources::thickness.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float2 screenCoord = (inputPixel.screen.xy + (surfaceNormal.xy * materialThickness));

    float materialClarity = Resources::clarity.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float3 glassColor = GetBiCubicSample(screenCoord, 1.0 - materialClarity);
    return (surfaceIrradiance + (glassColor * materialAlbedo));
}

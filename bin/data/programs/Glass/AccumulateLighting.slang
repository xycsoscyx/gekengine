#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>
#include <GEKLighting.slang>

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
    uint width, height, mipMapCount;
    Resources::glassBuffer.GetDimensions(0, width, height, mipMapCount);

    // Clamp explicit LOD to valid levels to avoid undefined sampling when runtime
    // mip count differs from the authored expectation.
    float maxMipMapLevel = ((mipMapCount > 0) ? float(mipMapCount - 1) : 0.0);
    float mipMapLevel = floor(glassLevel * maxMipMapLevel);
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    float2 pixelSize = (1.0f / float2(width, height));

    float fx = frac(screenCoord.x);
    float fy = frac(screenCoord.y);
    screenCoord.x -= fx;
    screenCoord.y -= fy;

    float4 xcubic = GetCubicFactors(fx);
    float4 ycubic = GetCubicFactors(fy);

    float4 c = screenCoord.xxyy + float2(-0.5, 1.5).xyxy;
    float4 s = float4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    float4 offset = c + float4(xcubic.yw, ycubic.yw) / s;
    offset *= pixelSize.xxyy * mipMapScale;

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

[shader("fragment")]
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    uint glassWidth, glassHeight, glassMipCount;
    Resources::glassBuffer.GetDimensions(0, glassWidth, glassHeight, glassMipCount);

    // Some backends provide SV_POSITION in pixel space, while others may surface
    // normalized/NDC-like values for this material path. Normalize to pixel space.
    float2 glassPixelCoord = inputPixel.screen.xy;
    if (abs(glassPixelCoord.x) <= 2.0f && abs(glassPixelCoord.y) <= 2.0f)
    {
        if (glassPixelCoord.x < 0.0f || glassPixelCoord.y < 0.0f || glassPixelCoord.x > 1.0f || glassPixelCoord.y > 1.0f)
        {
            glassPixelCoord = (glassPixelCoord * 0.5f) + 0.5f;
        }

        glassPixelCoord *= float2(glassWidth, glassHeight);
    }

    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord.xy);

    float3 surfacePosition = inputPixel.position;

    const float yFlipSign = (Options::Normals::FlipY ? -1.0 : 1.0);
    // PreferDDSDecode=true: BC5 RG decode. PreferDDSDecode=false: full RGB decode.
    const uint decodeMode = (Options::Normals::PreferDDSDecode ? 0u : 1u);

    float3 surfaceNormal = DecodeNormalSample(Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord.xy), yFlipSign, decodeMode);

    surfaceNormal = GetSurfaceNormalFromTangent(inputPixel.position, inputPixel.normal, inputPixel.texCoord.xy, surfaceNormal, inputPixel.tangent, inputPixel.biTangent);
    surfaceNormal = normalize((inputPixel.isFrontFacing != 0) ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);

    float materialClarity = Resources::clarity.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float glassLevel = (1.0f - materialClarity);
    if (Options::UseChromaticAbberation)
    {
        float materialThickness = Resources::thickness.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
        float2 screenCoordRed = (glassPixelCoord + (surfaceNormal.xy * materialThickness * -60.0f));
        float2 screenCoordGreen = (glassPixelCoord + surfaceNormal.xy);
        float2 screenCoordBlue = (glassPixelCoord + (surfaceNormal.xy * materialThickness * 60.0f));

        float glassColorRed = GetBiCubicSample(screenCoordRed, glassLevel).r;
        float glassColorGreen = GetBiCubicSample(screenCoordGreen, glassLevel).g;
        float glassColorBlue = GetBiCubicSample(screenCoordBlue, glassLevel).b;
        return (surfaceIrradiance + (float3(glassColorRed, glassColorGreen, glassColorBlue) * materialAlbedo));
    }
    else
    {
        float3 glassColor = GetBiCubicSample(glassPixelCoord, glassLevel);
        return (surfaceIrradiance + (glassColor * materialAlbedo));
    }
}

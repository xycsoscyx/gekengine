#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
OutputPixel mainPixelProgram(InputPixel inputPixel) : SV_Target0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;
    const float yFlipSign = (Options::Normals::FlipY ? -1.0f : 1.0f);
    // PreferDDSDecode=true: BC5 RG decode. PreferDDSDecode=false: full RGB decode.
    const uint decodeMode = (Options::Normals::PreferDDSDecode ? 0u : 1u);

    float3 surfaceNormal = DecodeNormalSample(Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord), yFlipSign, decodeMode);
    surfaceNormal = GetSurfaceNormalFromTangent(inputPixel.position, inputPixel.normal, inputPixel.texCoord, surfaceNormal, inputPixel.tangent, inputPixel.biTangent);
    surfaceNormal = normalize((inputPixel.isFrontFacing != 0) ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);

    OutputPixel outputPixel;
    outputPixel.finalBuffer = materialAlbedo;
    return outputPixel;
}

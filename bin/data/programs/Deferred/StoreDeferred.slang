#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }

    const float yFlipSign = (Options::Normals::FlipY ? -1.0 : 1.0);
    // PreferDDSDecode=true: BC5 RG decode. PreferDDSDecode=false: full RGB decode.
    const uint decodeMode = (Options::Normals::PreferDDSDecode ? 0u : 1u);

    float3 normal = DecodeNormalSample(Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord), yFlipSign, decodeMode);

    normal = GetSurfaceNormalFromTangent(inputPixel.position, inputPixel.normal, inputPixel.texCoord, normal, inputPixel.tangent, inputPixel.biTangent);
    normal = normalize((inputPixel.isFrontFacing != 0) ? normal : -normal);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.rgb;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = GetEncodedNormal(normal);
    return outputPixel;
}

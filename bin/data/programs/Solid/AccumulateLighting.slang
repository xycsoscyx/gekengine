#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>
#include <GEKLighting.slang>

[shader("fragment")]
OutputPixel mainPixelProgram(InputPixel inputPixel) : SV_Target0
{
    const uint debugSelection = uint(Options::BRDF::Debug::Selection);

    // --- Pre-texture debug modes: fire before any texture sampling or alpha discard ---
    switch (debugSelection)
    {
    case 11u: // ShowForceVisible
    {
        // Constant magenta: confirms geometry reaches the rasterizer and blit path delivers it.
        OutputPixel outputPixel;
        outputPixel.finalBuffer  = float3(1.0f, 0.0f, 1.0f);
        outputPixel.albedoBuffer = float3(1.0f, 0.0f, 1.0f);
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    case 12u: // ShowTexCoord
    {
        // UV coordinates as colour: red = U, green = V.
        float2 uv = frac(inputPixel.texCoord);
        OutputPixel outputPixel;
        outputPixel.finalBuffer  = float3(uv.x, uv.y, 0.0f);
        outputPixel.albedoBuffer = float3(uv.x, uv.y, 0.0f);
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    case 13u: // ShowWorldNormal
    {
        // Interpolated vertex normal remapped to [0,1]: confirms vertex interpolation.
        float3 n = inputPixel.normal * 0.5f + 0.5f;
        OutputPixel outputPixel;
        outputPixel.finalBuffer  = n;
        outputPixel.albedoBuffer = n;
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    }
    // ---------------------------------------------------------------------------------

    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;
    const float yFlipSign = (Options::Normals::FlipY ? -1.0f : 1.0f);
    // PreferDDSDecode=true assumes BC5-style RG normals.
    // PreferDDSDecode=false assumes full RGB normals (BC7/PNG/TGA authored XYZ).
    const uint decodeMode = (Options::Normals::PreferDDSDecode ? 0u : 1u);

    const float4 sampledNormal = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord);
    float3 textureNormal = DecodeNormalSample(sampledNormal, yFlipSign, decodeMode);
    float3 surfaceNormal = GetSurfaceNormalFromTangent(inputPixel.position, inputPixel.normal, inputPixel.texCoord, textureNormal, inputPixel.tangent, inputPixel.biTangent);
    float3 meshBasisNormal = GetSurfaceNormalFromMeshBasis(inputPixel.normal, inputPixel.tangent, inputPixel.biTangent, textureNormal);

    switch (debugSelection)
    {
    case 14u: // ShowSampledNormalChannels
    {
        OutputPixel outputPixel;
        outputPixel.finalBuffer = sampledNormal.rgb;
        outputPixel.albedoBuffer = sampledNormal.rgb;
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    case 15u: // ShowTangentNormal
    {
        float3 debugNormal = (textureNormal * 0.5f) + 0.5f;
        OutputPixel outputPixel;
        outputPixel.finalBuffer = debugNormal;
        outputPixel.albedoBuffer = debugNormal;
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    case 16u: // ShowMirrorHandedness
    {
        float handedness = GetCoTangentFrameHandedness(inputPixel.position, inputPixel.normal, inputPixel.texCoord);
        float3 debugColor = (handedness < 0.0f) ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 1.0f, 0.0f);
        OutputPixel outputPixel;
        outputPixel.finalBuffer = debugColor;
        outputPixel.albedoBuffer = debugColor;
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    case 17u: // ShowBasisDelta
    {
        float3 delta = abs(surfaceNormal - meshBasisNormal);
        OutputPixel outputPixel;
        outputPixel.finalBuffer = delta;
        outputPixel.albedoBuffer = delta;
        outputPixel.normalBuffer = float2(0.5f, 0.5f);
        return outputPixel;
    }
    }

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);

    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);

    OutputPixel outputPixel;
    outputPixel.finalBuffer = surfaceIrradiance;
    outputPixel.albedoBuffer = materialAlbedo;
    outputPixel.normalBuffer = GetEncodedNormal(surfaceNormal);
    return outputPixel;
}

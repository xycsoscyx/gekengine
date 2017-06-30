#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

// assume normals are stored as 3Dc format, so generate the Z value
float3 ConvertNormal(float2 encoded)
{
    float3 normal;
    normal.xy = ((encoded.xy * 2.0) - 1.0) * float2(1.0f, -1.0f);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    return normal;
}

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);

    float3 surfaceNormal = ConvertNormal(Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord).xy);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);
    surfaceNormal = mul(surfaceNormal, viewBasis);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);

    OutputPixel outputPixel;
    outputPixel.finalBuffer = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
    outputPixel.albedoBuffer = materialAlbedo;
    outputPixel.normalBuffer = GetEncodedNormal(surfaceNormal);
    return outputPixel;
}

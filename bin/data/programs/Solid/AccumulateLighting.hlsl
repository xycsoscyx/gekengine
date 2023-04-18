#include <GEKEngine>

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

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

    float3 normalColor = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord).xyz;
    float3 textureNormal = ((normalColor * (255.0 / 127.0)) - (128.0 / 127.0));
    float3 surfaceNormal = normalize((textureNormal.x * inputPixel.tangent) + 
                                     (textureNormal.y * inputPixel.biTangent) +
                                     (textureNormal.z * inputPixel.normal));
    surfaceNormal = normalColor;

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);

    OutputPixel outputPixel;
    outputPixel.finalBuffer = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
    outputPixel.albedoBuffer = materialAlbedo;
    outputPixel.normalBuffer = GetEncodedNormal(surfaceNormal);
    return outputPixel;
}

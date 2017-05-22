#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

static const float DetailMultiplier = 10.0f;

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
    const float4 albedo = float4(1.0f, 1.0f, 1.0f, 1.0f);// Resources::albedo.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;
    float detail = saturate(1.0f - (length(surfacePosition) / 10.0f));

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);

    float3 surfaceNormal = ConvertNormal(Resources::normal.Sample(Global::LinearWrapSampler, inputPixel.texCoord).xy);
    surfaceNormal += (ConvertNormal(Resources::detail.Sample(Global::LinearWrapSampler, inputPixel.texCoord * 5.0f).xy) * detail);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);
    surfaceNormal = mul(surfaceNormal, viewBasis);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::LinearWrapSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    OutputPixel outputPixel;
    outputPixel.screen = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
    outputPixel.normalBuffer = getEncodedNormal(surfaceNormal);
    return outputPixel;
}

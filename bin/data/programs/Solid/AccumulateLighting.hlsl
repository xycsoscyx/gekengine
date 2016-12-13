#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);

    float3 surfaceNormal;
    // assume normals are stored as 3Dc format, so generate the Z value
    surfaceNormal.xy = Resources::normal.Sample(Global::LinearWrapSampler, inputPixel.texCoord);
    surfaceNormal.xy = ((surfaceNormal.xy * 2.0) - 1.0);
    surfaceNormal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    surfaceNormal.z = sqrt(1.0 - dot(surfaceNormal.xy, surfaceNormal.xy));
    surfaceNormal = mul(surfaceNormal, viewBasis);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::LinearWrapSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    OutputPixel outputPixel;
    outputPixel.finalBuffer = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
    outputPixel.normalBuffer = getEncodedNormal(surfaceNormal);
    return outputPixel;
}

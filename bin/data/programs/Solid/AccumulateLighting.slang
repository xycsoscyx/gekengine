#include <GEKEngine>
#include <GEKGlobal.slang>
#include <GEKUtility.slang>
#include <GEKLighting.slang>

[shader("fragment")]
OutputPixel mainPixelProgram(InputPixel inputPixel) : SV_Target0
{
    const uint debugSelection = uint(Options::BRDF::Debug::Selection);

    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);

    [branch]
    if (debugSelection != 18u && albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;
    const float yFlipSign = (Options::Normals::FlipY ? -1.0f : 1.0f);
    const uint decodeMode = (Options::Normals::PreferDDSDecode ? 0u : 1u);

    const float4 sampledNormal = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord);
    float3 textureNormal = DecodeNormalSample(sampledNormal, yFlipSign, decodeMode);
    // Flip X for mirrored tangent space
    if (inputPixel.tangent.w < 0.0f)
    {
        textureNormal.x = -textureNormal.x;
    }
    float3 coTangentSurfaceNormal = GetSurfaceNormalFromTangent(inputPixel.position, inputPixel.normal, inputPixel.texCoord, textureNormal, inputPixel.tangent, inputPixel.biTangent);
    float3 meshBasisNormal = GetSurfaceNormalFromMeshBasis(inputPixel.normal, inputPixel.tangent, inputPixel.biTangent, textureNormal);
    float3 surfaceNormal = coTangentSurfaceNormal;
    surfaceNormal = normalize((inputPixel.isFrontFacing != 0) ? surfaceNormal : -surfaceNormal);

    switch (debugSelection)
    {
        case 19u: // ShowTangentHandedness
        {
            float w = inputPixel.tangent.w;
            float3 debugColor = (w > 0.0f) ? float3(0,1,0) : ((w < 0.0f) ? float3(1,0,0) : float3(0,0,1));
            OutputPixel outputPixel;
            outputPixel.finalBuffer = debugColor;
            outputPixel.albedoBuffer = debugColor;
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
        case 11u: // ShowForceVisible
        {
            OutputPixel outputPixel;
            outputPixel.finalBuffer  = float3(1.0f, 0.0f, 1.0f);
            outputPixel.albedoBuffer = float3(1.0f, 0.0f, 1.0f);
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
        case 12u: // ShowTexCoord
        {
            float2 uv = frac(inputPixel.texCoord);
            OutputPixel outputPixel;
            outputPixel.finalBuffer  = float3(uv.x, uv.y, 0.0f);
            outputPixel.albedoBuffer = float3(uv.x, uv.y, 0.0f);
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
        case 13u: // ShowWorldNormal
        {
            float3 n = inputPixel.normal * 0.5f + 0.5f;
            OutputPixel outputPixel;
            outputPixel.finalBuffer  = n;
            outputPixel.albedoBuffer = n;
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
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
            float handedness = dot(cross(normalize(inputPixel.tangent.xyz), normalize(inputPixel.biTangent)), normalize(inputPixel.normal));
            float3 debugColor = (handedness < 0.0f) ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 1.0f, 0.0f);
            OutputPixel outputPixel;
            outputPixel.finalBuffer = debugColor;
            outputPixel.albedoBuffer = debugColor;
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
        case 17u: // ShowBasisDelta
        {
            float3 delta = abs(coTangentSurfaceNormal - meshBasisNormal);
            OutputPixel outputPixel;
            outputPixel.finalBuffer = delta;
            outputPixel.albedoBuffer = delta;
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
        case 18u: // ShowTangentDebugTriplet
        {
            float3 debugA = (textureNormal * 0.5f) + 0.5f;
            float sign = (inputPixel.tangent.w < 0.0f) ? -1.0f : ((inputPixel.tangent.w > 0.0f) ? 1.0f : 0.0f);
            float3 debugB = (sign < 0.0f) ? float3(1,0,0) : ((sign > 0.0f) ? float3(0,1,0) : float3(0,0,1));
            float3 debugC = (surfaceNormal * 0.5f) + 0.5f;
            float2 uv = frac(inputPixel.screen.xy / float2(1920,1080));
            float3 debugColor = (uv.x < 0.33f) ? debugA : ((uv.x < 0.66f) ? debugB : debugC);
            bool bad = any(isnan(debugColor)) || any(debugColor < 0.0f) || any(debugColor > 1.0f);
            debugColor = bad ? float3(1,0,1) : debugColor;
            OutputPixel outputPixel = (OutputPixel)0;
            outputPixel.finalBuffer = debugColor;
            outputPixel.albedoBuffer = debugColor;
            outputPixel.normalBuffer = float2(0.5f, 0.5f);
            return outputPixel;
        }
    }

    // Default lighting path
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

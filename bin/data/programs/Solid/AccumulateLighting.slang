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

    float3 normalColor = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord).xyz;
    float3 textureNormal = ((normalColor * (255.0 / 127.0)) - (128.0 / 127.0));
    
    float3 surfaceNormal = normalize((textureNormal.x * inputPixel.tangent) + 
                                     (textureNormal.y * inputPixel.biTangent) +
                                     (textureNormal.z * inputPixel.normal));
    //surfaceNormal = inputPixel.normal;

    //float3x3 coTangentFrame = GetCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);
    //surfaceNormal = mul(coTangentFrame, textureNormal);

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

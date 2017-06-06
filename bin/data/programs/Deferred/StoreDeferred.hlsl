#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
	//float3x3 viewBasis = GetCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord).xy;
    normal.xy = ((normal.xy * 2.0) - 1.0);
	normal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = mul(normal, viewBasis);
    normal = normalize(inputPixel.isFrontFacing ? normal : -normal);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.rgb;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = GetEncodedNormal(normal);
    return outputPixel;
}

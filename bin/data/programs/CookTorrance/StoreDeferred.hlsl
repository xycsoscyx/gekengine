#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be in sRGB format and converted to linear automatically
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }

    float3x3 viewBasis = getCoTangentFrame(inputPixel.viewPosition, inputPixel.viewNormal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    normal.xy = ((normal.xy * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = mul(normal, viewBasis);
    normal = normalize(inputPixel.frontFacing ? normal : -normal);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.rgb;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metalness.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    return outputPixel;
}
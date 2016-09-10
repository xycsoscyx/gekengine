#include "GEKShader"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be in sRGB format and converted to linear automatically
    float4 albedo = Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }


	float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
	//float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    normal.xy = ((normal.xy * 2.0) - 1.0);
	normal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = mul(normal, viewBasis);
    normal = normalize(inputPixel.isFrontFacing ? normal : -normal);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.rgb;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metallic.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    return outputPixel;
}

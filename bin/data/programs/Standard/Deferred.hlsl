#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be in sRGB format and converted to linear automatically
    float4 albedo = (Resources::albedo.Sample(Global::linearSampler, inputPixel.texCoord) * inputPixel.color);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }

    float3x3 viewBasis = getCoTangentFrame(inputPixel.viewPosition, inputPixel.viewNormal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = ((Resources::normal.Sample(Global::linearSampler, inputPixel.texCoord) * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = (mul(normal, viewBasis)) * (inputPixel.frontFacing ? 1 : -1);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.xyz;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metalness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    outputPixel.depthBuffer  = (inputPixel.viewPosition.z / Camera::maximumDistance);
    return outputPixel;
}

#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

OutputPixel mainPixelProgram(in InputPixel inputPixel)
{
    float4 albedo = (Resources::albedo.Sample(Global::linearSampler, inputPixel.texCoord) * inputPixel.color);
    
    [branch]
    if(albedo.a < 0.5f)
    {
        discard;
    }

    float3x3 viewBasis = getCoTangentFrame(inputPixel.viewPosition, inputPixel.viewNormal, inputPixel.texCoord);

    float3 normal;
    normal = ((Resources::normal.Sample(Global::linearSampler, inputPixel.texCoord) * 255.0 / 127.0) - 128.0 / 127.0);
    //normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = normalize(mul(normalize(normal), viewBasis)) * (inputPixel.frontFacing ? 1 : -1);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.xyz;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metalness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    outputPixel.depthBuffer  = (inputPixel.viewPosition.z / Camera::maximumDistance);
    return outputPixel;
}

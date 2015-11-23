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

    float3 viewNormal = (normalize(inputPixel.viewNormal) * (inputPixel.frontFacing ? 1 : -1));
    float3x3 coTangentFrame = getCoTangentFrame(inputPixel.viewPosition.xyz, viewNormal, inputPixel.texCoord);

    float3 normal;
    normal.xy = ((Resources::normal.Sample(Global::linearSampler, inputPixel.texCoord) * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = normalize(mul(normal, coTangentFrame));

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.xyz;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.materialBuffer.y = Resources::metalness.Sample(Global::linearSampler, inputPixel.texCoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    outputPixel.depthBuffer  = (inputPixel.viewPosition.z / Camera::maximumDistance);
    return outputPixel;
}

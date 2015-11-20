#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

OutputPixel mainPixelProgram(in InputPixel inputPixel)
{
    float4 albedo = (Resources::albedo.Sample(Global::linearSampler, inputPixel.texcoord) * inputPixel.color);
    
    [branch]
    if(albedo.a < 0.5f)
    {
        discard;
    }

    float3x3 coTangentFrame = getCoTangentFrame(inputPixel.viewposition.xyz, (normalize(inputPixel.viewnormal) * (inputPixel.frontface ? 1 : -1)), inputPixel.texcoord);

    float3 normal;
    // Normals stored as 3Dc format, so [0,1] XY components only
    normal.xy = ((Resources::normal.Sample(Global::linearSampler, inputPixel.texcoord) * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = mul(normalize(normal), coTangentFrame);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = albedo.xyz;
    outputPixel.materialBuffer.x = Resources::roughness.Sample(Global::linearSampler, inputPixel.texcoord);
    outputPixel.materialBuffer.y = Resources::metalness.Sample(Global::linearSampler, inputPixel.texcoord);
    outputPixel.normalBuffer = encodeNormal(normal);
    outputPixel.depthBuffer  = (inputPixel.viewposition.z / Camera::maximumDistance);
    return outputPixel;
}

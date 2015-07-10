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

    // Viewspace vertex normal
    float3 viewNormal = (normalize(inputPixel.viewnormal) * (inputPixel.frontface ? 1 : -1));
                
    float3 normal;
    // Normals stored as 3Dc format, so [0,1] XY components only
    normal.xy = ((Resources::normal.Sample(Global::linearSampler, inputPixel.texcoord).yx * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));

    float3x3 coTangentFrame = getCoTangentFrame(-inputPixel.viewposition.xyz, viewNormal, inputPixel.texcoord);
    normal = mul(normal, coTangentFrame);

    OutputPixel outputPixel;
    outputPixel.albedoBuffer = (albedo * Material::color);
    outputPixel.normalBuffer = encodeNormal(normal);
    outputPixel.depthBuffer  = (inputPixel.viewposition.z / Camera::maximumDistance);
    outputPixel.infoBuffer   = Resources::info.Sample(Global::linearSampler, inputPixel.texcoord);
    return outputPixel;
}

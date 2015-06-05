#include "..\GEKEngine.h"
#include "..\GEKTypes.h"
#include "..\GEKUtility.h"

struct OutputPixel
{
    float4 albedo : SV_TARGET0;
    float2 normal : SV_TARGET1;
    float  depth : SV_TARGET2;
    float4 info : SV_TARGET3;
};

OutputPixel mainPixelProgram(in InputPixel inputPixel)
{
    float4 albedo = (Maps::albedo.Sample(Global::linearSampler, inputPixel.texcoord) * inputPixel.color);

    [branch]
    if(albedo.a < 0.5f)
    {
        discard;
    }

    // Viewspace vertex normal
    float3 viewNormal = (normalize(inputPixel.viewnormal) * (inputPixel.frontface ? 1 : -1));
                
    float3 normal;
    // Normals stored as 3Dc format, so [0,1] XY components only
    normal.xy = ((Material::Maps::normal.Sample(Global::linearSampler, inputPixel.texcoord).yx * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));

    float3x3 coTangentFrame = getCoTangentFrame(-inputPixel.viewposition.xyz, viewNormal, inputPixel.texcoord);
    normal = mul(normal, coTangentFrame);

    OutputPixel outputPixel;
    outputPixel.albedo.xyz = albedo.xyz;
    outputPixel.albedo.a   = Material::Properties::color;
    outputPixel.normal     = encodeNormal(normal);
    outputPixel.depth      = (inputPixel.viewposition.z / Camera::maximumDistance);
    outputPixel.info       = Material::Maps::info.Sample(Global::linearSampler, inputPixel.texcoord);
    return outputPixel;
}

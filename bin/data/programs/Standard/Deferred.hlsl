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
    float4 nAlbedo = (Maps::albedo.Sample(Global::linearSampler, inputPixel.texcoord) * inputPixel.color);

    [branch]
    if(nAlbedo.a < 0.5f)
    {
        discard;
    }

    // Viewspace vertex normal
    float3 nViewNormal = (normalize(inputPixel.viewnormal) * (inputPixel.frontface ? 1 : -1));
                
    float3 nNormal;
    // Normals stored as 3Dc format, so [0,1] XY components only
    nNormal.xy = ((gs_pNormalMap.Sample(gs_pLinearSampler, inputPixel.texcoord).yx * 2.0) - 1.0);
    nNormal.z = sqrt(1.0 - dot(nNormal.xy, nNormal.xy));

    float3x3 nCoTangentFrame = GetCoTangentFrame(-inputPixel.viewposition.xyz, nViewNormal, inputPixel.texcoord);
    nNormal = mul(nNormal, nCoTangentFrame);

    OutputPixel outputPixel;
    outputPixel.albedo.xyz = nAlbedo.xyz;
    outputPixel.albedo.a   = 0;//gs_bMaterialFullBright;
    outputPixel.normal     = EncodeNormal(nNormal);
    outputPixel.depth      = (inputPixel.viewposition.z / Camera::maximumDistance);
    outputPixel.info       = gs_pInfoMap.Sample(gs_pLinearSampler, inputPixel.texcoord);
    return outputPixel;
}

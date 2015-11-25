#include "GEKGlobal.h"
#include "GEKtypes.h"
#include "GEKmaterial.h"
#include "GEKutility.h"

Texture2D     gs_pAlbedoMap             : register(t0);
Texture2D     gs_pNormalMap             : register(t1);

struct OUTPUT
{
    float4 albedo                       : SV_TARGET0;
    float2 normal                       : SV_TARGET1;
    float  depth                        : SV_TARGET2;
};

OUTPUT MainPixelProgram(in INPUT kInput)
{
    float4 nAlbedo = (gs_pAlbedoMap.Sample(gs_pLinearSampler, kInput.texCoord) * kInput.color);

    [branch]
    if(nAlbedo.a < 0.5f)
    {
        discard;
    }

    // Viewspace vertex normal
    float3 nViewNormal = (normalize(kInput.viewNormal) * (kInput.frontFacing ? 1 : -1));
                
    float3 nNormal;
    // Normals stored as 3Dc format, so [0,1] XY components only
    nNormal.xy = ((gs_pNormalMap.Sample(gs_pLinearSampler, kInput.texCoord).yx * 2.0) - 1.0);
    nNormal.z = sqrt(1.0 - dot(nNormal.xy, nNormal.xy));

    float3x3 nCoTangentFrame = GetCoTangentFrame(-kInput.viewPosition.xyz, nViewNormal, kInput.texCoord);
    nNormal = mul(nNormal, nCoTangentFrame);

    OUTPUT kOutput;
    kOutput.albedo.xyz = nAlbedo.xyz;
    kOutput.albedo.a   = 0;//gs_bMaterialFullBright;
    kOutput.normal     = EncodeNormal(nNormal);
    kOutput.depth      = (kInput.viewPosition.z / Camera::maximumDistance);
    return kOutput;
}

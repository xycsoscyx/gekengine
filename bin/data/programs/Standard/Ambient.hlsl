#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.h"

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float4 albedoTerm = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    return albedoTerm;

    float3 lightingContribution = 0.0f;

    [branch]
    if (albedoTerm.a < 1.0f)
    {
        float4 pixelInfo = Resources::infoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

        float pixelDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
        float3 pixelPosition = getViewPosition(inputPixel.texcoord, pixelDepth);
        float3 pixelNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord));

        float3 viewNormal = -normalize(pixelPosition);

        float3 lightColor = ambientMap.Sample(Global::pointSampler, pixelNormal);
        float3 diffuseContribution = 0.0f;
        float3 specularContribution = 0.0f;
        getBRDF(albedoTerm, pixelNormal, -pixelNormal, viewNormal, pixelInfo, diffuseContribution, specularContribution);
        lightingContribution = saturate(lightColor * diffuseContribution) + saturate(lightColor * specularContribution);
    }

    return float4(lightingContribution, albedoTerm.a);
}
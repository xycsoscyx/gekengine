#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.h"

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float4 albedoTerm = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

    float3 lightingContribution = 0.0f;

    [branch]
    if (albedoTerm.a < 1.0f)
    {
        float4 pixelInfo = Resources::infoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

        float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
        float3 surfacePosition = getViewPosition(inputPixel.texcoord, surfaceDepth);
        float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord));

        float3 viewDirection = -normalize(surfacePosition);

        float3x3 inverseViewMatrix = (float3x3)Camera::viewMatrix;
        float3 lightDirection = -mul(inverseViewMatrix, surfaceNormal);
        float3 lightColor = Resources::ambientLightMap.Sample(Global::pointSampler, -lightDirection);

        float3 diffuseContribution = 0.0f;
        float3 specularContribution = 0.0f;
        getBRDF(albedoTerm, surfaceNormal, lightDirection, viewDirection, pixelInfo, diffuseContribution, specularContribution);
        lightingContribution = saturate(lightColor * diffuseContribution) + saturate(lightColor * specularContribution);
    }

    return float4(lightingContribution, albedoTerm.a);
}
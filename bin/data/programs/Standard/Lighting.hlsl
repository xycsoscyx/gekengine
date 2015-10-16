#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

// Diffuse & Specular Term
// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
bool getBRDF(in float3 albedoTerm, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal, in float4 pixelInfo, out float3 diffuseContribution, out float3 specularContribution)
{
    float angleCenterLight = saturate(dot(pixelNormal, lightNormal));

    float materialRoughness = ((pixelInfo.x * 0.9) + 0.1);
    materialRoughness = (materialRoughness * materialRoughness * 2);
    float materialRoughnessSquared = (materialRoughness * materialRoughness);
    float materialSpecular = pixelInfo.y;
    float materialMetalness = pixelInfo.z;

    float3 Ks = lerp(materialSpecular, albedoTerm, materialMetalness);
    float3 Kd = lerp(albedoTerm, 0, materialMetalness);
    float3 Fd = 1.0 - Ks;

    float angleCenterView = dot(pixelNormal, viewNormal);
    float angleCenterViewSquared = (angleCenterView * angleCenterView);
    float inverseAngleCenterViewSquared = (1.0 - angleCenterViewSquared);

    float3 halfVector = normalize(lightNormal + viewNormal);
    float angleCenterHalf = saturate(dot(pixelNormal, halfVector));
    float angleLightHalf = saturate(dot(lightNormal, halfVector));

    float angleCenterLightSquared = (angleCenterLight * angleCenterLight);
    float angleCenterHalfSquared = (angleCenterHalf * angleCenterHalf);
    float inverseAngleCenterLightSquared = (1.0 - angleCenterLightSquared);

    diffuseContribution = (Kd * Fd * Math::ReciprocalPi * saturate((1 - materialRoughness) * 0.5 + 0.5 + materialRoughnessSquared * (8 - materialRoughness) * 0.023));

    float centeredAngleCenterView = (angleCenterView * 0.5 + 0.5);
    float centeredAngleCenterLight = (angleCenterLight * 0.5 + 0.5);
    angleCenterViewSquared = (centeredAngleCenterView * centeredAngleCenterView);
    angleCenterLightSquared = (centeredAngleCenterLight * centeredAngleCenterLight);
    inverseAngleCenterViewSquared = (1.0 - angleCenterViewSquared);
    inverseAngleCenterLightSquared = (1.0 - angleCenterLightSquared);

    float diffuseDelta = lerp((1 / (0.1 + materialRoughness)), (-materialRoughnessSquared * 2), saturate(materialRoughness));
    float diffuseViewAngle = (1 - (pow(1 - centeredAngleCenterView, 4) * diffuseDelta));
    float diffuseLightAngle = (1 - (pow(1 - centeredAngleCenterLight, 4) * diffuseDelta));
    diffuseContribution *= (diffuseLightAngle * diffuseViewAngle * angleCenterLight);

    float3 Fs = (Ks + Fd * pow((1 - angleLightHalf), 5));
    float3 D = (pow(materialRoughness / (angleCenterHalfSquared * (materialRoughnessSquared + (1 - angleCenterHalfSquared) / angleCenterHalfSquared)), 2) * Math::ReciprocalPi);
    specularContribution = (Fs * D * angleCenterLight);
    return true;
}

float3 getLightingContribution(in InputPixel inputPixel, in float3 albedoTerm)
{
    float4 pixelInfo = Resources::infoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

    float pixelDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float3 pixelPosition = getViewPosition(inputPixel.texcoord, pixelDepth);
    float3 pixelNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord));

    float3 viewNormal = -normalize(pixelPosition);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(lightTileSize).xx));
    const uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
    const uint bufferIndex = (tileIndex * Lighting::listSize);

    float3 totalDiffuseContribution = 0.0f;
    float3 totalSpecularContribution = 0.0f;

    [loop]
    for (uint lightIndexIndex = 0; lightIndexIndex < Lighting::count; lightIndexIndex++)
    {
        uint lightIndex = Resources::tileIndexList[bufferIndex + lightIndexIndex];

        [branch]
        if (lightIndex == Lighting::listSize)
        {
            break;
        }

        float3 lightVector = (Lighting::list[lightIndex].position.xyz - pixelPosition);
        float lightDistance = length(lightVector);
        float3 lightNormal = normalize(lightVector);

        float attenuation = (1.0f - saturate(lightDistance / Lighting::list[lightIndex].range));

        [branch]
        if (attenuation > 0.0f)
        {
            float3 diffuseContribution = 0.0f;
            float3 specularContribution = 0.0f;

            [branch]
            if (getBRDF(albedoTerm, pixelNormal, lightNormal, viewNormal, pixelInfo, diffuseContribution, specularContribution))
            {
                totalDiffuseContribution += saturate(Lighting::list[lightIndex].color * diffuseContribution * attenuation);
                totalSpecularContribution += saturate(Lighting::list[lightIndex].color * specularContribution * attenuation);
            }
        }
    }

    return (saturate(totalDiffuseContribution) + saturate(totalSpecularContribution));
}

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    return Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord).xyxy;
    float4 albedoTerm = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

    float3 lightingContribution = 0.0f;

    [branch]
    if (albedoTerm.a < 1.0f)
    {
        lightingContribution = getLightingContribution(inputPixel, albedoTerm.xyz);
    }

    return float4(lightingContribution, albedoTerm.a);
}

#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#ifdef _COMPUTE_PROGRAM

groupshared uint    tileMinimumDepth;
groupshared uint    tileMaximumDepth;
groupshared uint    tileLightCount;
groupshared uint    tileLightList[Lighting::listSize];
groupshared float4  tileFrustum[6];

[numthreads(uint(lightTileSize), uint(lightTileSize), 1)]
void mainComputeProgram(uint3 screenPosition : SV_DispatchThreadID, uint3 tilePosition : SV_GroupID, uint pixelIndex : SV_GroupIndex)
{
    [branch]
    if (pixelIndex == 0)
    {
        tileLightCount = 0;
        tileMinimumDepth = 0x7F7FFFFF;
        tileMaximumDepth = 0;
    }

    float viewDepth = Resources::depthBuffer[screenPosition.xy] * Camera::maximumDistance;
    uint viewDepthInteger = asuint(viewDepth);

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(tileMinimumDepth, viewDepthInteger);
    InterlockedMax(tileMaximumDepth, viewDepthInteger);

    GroupMemoryBarrierWithGroupSync();

    float minimumDepth = asfloat(tileMinimumDepth);
    float maximumDepth = asfloat(tileMaximumDepth);

    [branch]
    if (pixelIndex == 0)
    {
        float2 depthBufferSize;
        Resources::depthBuffer.GetDimensions(depthBufferSize.x, depthBufferSize.y);
        float2 tileScale = depthBufferSize * rcp(float(2 * lightTileSize));
        float2 tileBias = tileScale - float2(tilePosition.xy);

        float3 frustumXPlane = float3(Camera::projectionMatrix[0][0] * tileScale.x, 0.0f, tileBias.x);
        float3 frustumYPlane = float3(0.0f, -Camera::projectionMatrix[1][1] * tileScale.y, tileBias.y);
        float3 frustumZPlane = float3(0.0f, 0.0f, 1.0f);

        tileFrustum[0] = float4(normalize(frustumZPlane - frustumXPlane), 0.0f),
        tileFrustum[1] = float4(normalize(frustumZPlane + frustumXPlane), 0.0f),
        tileFrustum[2] = float4(normalize(frustumZPlane - frustumYPlane), 0.0f),
        tileFrustum[3] = float4(normalize(frustumZPlane + frustumYPlane), 0.0f),
        tileFrustum[4] = float4(0.0f, 0.0f, 1.0f, -minimumDepth);
        tileFrustum[5] = float4(0.0f, 0.0f, -1.0f, maximumDepth);
    }

    GroupMemoryBarrierWithGroupSync();

    [loop]
    for (uint lightIndex = pixelIndex; lightIndex < Lighting::count; lightIndex += (lightTileSize * lightTileSize))
    {
        bool isLightVisible = true;

        [unroll]
        for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            float lightDistance = dot(tileFrustum[planeIndex], float4(Lighting::list[lightIndex].position, 1.0f));
            isLightVisible = (isLightVisible && (lightDistance >= -Lighting::list[lightIndex].range));
        }

        [branch]
        if (isLightVisible)
        {
            uint tileIndex;
            InterlockedAdd(tileLightCount, 1, tileIndex);
            tileLightList[tileIndex] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if (pixelIndex < Lighting::listSize)
    {
        uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
        uint bufferIndex = ((tileIndex * Lighting::listSize) + pixelIndex);
        uint lightIndex = (pixelIndex < tileLightCount ? tileLightList[pixelIndex] : Lighting::listSize);
        UnorderedAccess::tileIndexList[bufferIndex] = lightIndex;
    }
}

#endif

#ifdef _PIXEL_PROGRAM

// Diffuse & Specular Term
// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
bool getBRDF(in float3 albedoTerm, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal, in float4 pixelInfo, out float3 diffuseContribution, out float3 specularContribution)
{
    float angleCenterLight = saturate(dot(pixelNormal, lightNormal));

    [branch]
    if (angleCenterLight <= 0.0f)
    {
        return false;
    }

    float materialRoughness = (pixelInfo.x * pixelInfo.x * 4);
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
    float3 lightingContribution = 0.0f;
    float4 albedoTerm = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    return albedoTerm;

    [branch]
    if (albedoTerm.a < 1.0f)
    {
        lightingContribution = getLightingContribution(inputPixel, albedoTerm.xyz);
    }

    return float4(lightingContribution, albedoTerm.a);
}

#endif
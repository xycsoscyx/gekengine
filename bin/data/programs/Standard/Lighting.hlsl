#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.h"

float3 getLightingContribution(in InputPixel inputPixel, in float3 materialAlbedo)
{
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

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

            getBRDF(materialAlbedo, materialInfo, pixelNormal, lightNormal, viewNormal, diffuseContribution, specularContribution);
            totalDiffuseContribution += saturate(Lighting::list[lightIndex].color * diffuseContribution * attenuation);
            totalSpecularContribution += saturate(Lighting::list[lightIndex].color * specularContribution * attenuation);
        }
    }

    return (saturate(totalDiffuseContribution) + saturate(totalSpecularContribution));
}

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float4 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);

    float3 lightingContribution = 0.0f;

    [branch]
    if (materialAlbedo.a < 1.0f)
    {
        lightingContribution = getLightingContribution(inputPixel, materialAlbedo.xyz);
    }

    return float4(lightingContribution, materialAlbedo.a);
}

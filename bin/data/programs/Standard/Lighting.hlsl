#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Disney.h"

float3 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float materialRoughness = materialInfo.x;
    float materialMetalness = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float3 surfacePosition = getViewPosition(inputPixel.texcoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord));

    float3 viewDirection = normalize(surfacePosition);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(lightTileSize).xx));
    const uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
    const uint bufferOffset = (tileIndex * Lighting::listSize);

    float3 surfaceColor = 0.0f;

    [loop]
    for (uint lightTileIndex = 0; lightTileIndex < Lighting::count; lightTileIndex++)
    {
        uint lightIndex = Resources::tileIndexList[bufferOffset + lightTileIndex];

        [branch]
        if (lightIndex == Lighting::listSize)
        {
            break;
        }

        float3 lightRay = (Lighting::list[lightIndex].position.xyz - surfacePosition);
        float lightDistance = length(lightRay);
        float3 lightDirection = normalize(lightRay);

        float attenuation = (lightDistance / Lighting::list[lightIndex].radius);

#ifdef _INVERSE_SQUARE
        attenuation = ???
#else
        attenuation = (1.0f - saturate(attenuation));
#endif

        [branch]
        if (attenuation > 0.0f)
        {
            float NdotL = dot(surfaceNormal, lightDirection);
            float NdotV = dot(surfaceNormal, viewDirection);
            surfaceColor += (NdotL * Lighting::list[lightIndex].color * attenuation);
            continue;

            [branch]
            if (NdotL > 0.0f && NdotV > 0.0f)
            {
                float3 lightContribution = getBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL, NdotV);
                surfaceColor += (lightContribution * NdotL * Lighting::list[lightIndex].color * attenuation);
            }
        }
    }

    return surfaceColor;
}

#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.UE4.h"

float3 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float materialRoughness = materialInfo.x;
    float materialMetalness = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));

    float3 viewDirection = -normalize(surfacePosition);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(tileSize).xx));
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

        float3 lightRay = -(surfacePosition - Lighting::list[lightIndex].position);
        float lightDistance = length(lightRay);
        float3 lightDirection = (lightRay / lightDistance);

#define _INVERSE_SQUARE 1

#if _INVERSE_SQUARE
        float attenuation = (lightDistance * lightDistance / (Lighting::list[lightIndex].radius * Lighting::list[lightIndex].radius));
        attenuation *= attenuation;
#else
        float attenuation = (lightDistance / Lighting::list[lightIndex].radius);
#endif

        attenuation = (1.0f - saturate(attenuation));

        [branch]
        if (attenuation > 0.0f)
        {
            float NdotL = dot(surfaceNormal, lightDirection);
            float3 lightContribution = getBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL);
            surfaceColor += (NdotL * lightContribution * attenuation * Lighting::list[lightIndex].color);
        }
    }

    return surfaceColor;
}

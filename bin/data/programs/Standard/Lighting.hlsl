#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Custom.h"

//#define _AREA_LIGHT_SIZE 1.0f
//#define _INVERSE_SQUARE 1

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float materialRoughness = ((materialInfo.x * 0.9f) + 0.1f); // account for infinitely small point lights
    float materialMetalness = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));

    float3 viewDirection = -normalize(surfacePosition);
#ifdef _AREA_LIGHT_SIZE
    float3 reflectNormal = reflect(-viewDirection, surfaceNormal);
#endif

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

#ifdef _AREA_LIGHT_SIZE
        float3 lightDirection = Lighting::list[lightIndex].position - surfacePosition;
        float3 centerToRay = dot(lightDirection, reflectNormal) * reflectNormal - lightDirection;
        float3 closestPoint = lightDirection + centerToRay * clamp(_AREA_LIGHT_SIZE / length(centerToRay), 0.0, 1.0);
        float lightDistanceSquared = dot(closestPoint, closestPoint);
        float lightDistance = sqrt(lightDistanceSquared);
        lightDirection = (closestPoint / lightDistance);
#else
        float3 lightRay = -(surfacePosition - Lighting::list[lightIndex].position);
        float lightDistanceSquared = dot(lightRay, lightRay);
        float lightDistance = sqrt(lightDistanceSquared);
        float3 lightDirection = (lightRay / lightDistance);
#endif

#if _INVERSE_SQUARE
        float falloff = (lightDistanceSquared / (Lighting::list[lightIndex].range * Lighting::list[lightIndex].range));
        falloff *= falloff;
        falloff = (1.0f - saturate(falloff));
#else
        float falloff = (lightDistance / Lighting::list[lightIndex].range);
        falloff = (1.0f - saturate(falloff));
#endif

        float NdotL = dot(surfaceNormal, lightDirection);

        float3 lightContribution = getBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL);
        surfaceColor += (saturate(NdotL) * lightContribution * falloff * Lighting::list[lightIndex].color);
    }

    return surfaceColor;
}

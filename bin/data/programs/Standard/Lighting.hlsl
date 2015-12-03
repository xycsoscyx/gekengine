#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Custom.h"


float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float materialRoughness = ((materialInfo.x * 0.9f) + 0.1f); // account for infinitely small point lights
    float materialMetalness = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));

    float3 viewDirection = -normalize(surfacePosition);
    float3 r = reflect(-viewDirection, surfaceNormal);

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

#ifdef _AREA_LIGHTS
        float3 L = Lighting::list[lightIndex].position - surfacePosition;
        float3 centerToRay = dot(L, r) * r - L;
        float3 closestPoint = L + centerToRay * clamp(5.0f / length(centerToRay), 0.0, 1.0);
        float lightDistanceSquared = dot(closestPoint, closestPoint);
        float lightDistance = sqrt(lightDistanceSquared);
        float3 lightDirection = (closestPoint / lightDistance);
#else
        float3 lightRay = -(surfacePosition - Lighting::list[lightIndex].position);
        float lightDistanceSquared = dot(lightRay, lightRay);
        float lightDistance = sqrt(lightDistanceSquared);
        float3 lightDirection = (lightRay / lightDistance);
#endif

#define _INVERSE_SQUARE 1

#if _INVERSE_SQUARE
        float attenuation = (lightDistanceSquared / (Lighting::list[lightIndex].radius * Lighting::list[lightIndex].radius));
        attenuation *= attenuation;
        attenuation = (1.0f - saturate(attenuation));
#else
        float attenuation = (lightDistance / Lighting::list[lightIndex].radius);
        attenuation = (1.0f - saturate(attenuation));
#endif

        [branch]
        if (attenuation > 0.0f)
        {
            float NdotL = dot(surfaceNormal, lightDirection);

            [branch]
            if (NdotL > 0.0f)
            {
                float3 lightContribution = getBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL);
                surfaceColor += (NdotL * lightContribution * attenuation * Lighting::list[lightIndex].color);
            }
        }
    }

    static const float3 luminance = float3(0.299f, 0.587f, 0.114f);
    float surfaceLuminance = dot(surfaceColor, luminance);
    return float4(surfaceColor, log2(surfaceLuminance));
}

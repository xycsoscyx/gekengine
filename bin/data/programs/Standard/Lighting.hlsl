#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Custom.h"

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float materialRoughness = ((materialInfo.x * 0.9) + 0.1); // account for infinitely small point lights
    float materialMetalness = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));

    float3 viewDirection = -normalize(surfacePosition);
    float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(tileSize).xx));
    const uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
    const uint bufferOffset = (tileIndex * Lighting::maximumListSize);

    float3 surfaceColor = 0.0;

    [loop]
    for (uint lightTileIndex = 0; lightTileIndex < Lighting::count; lightTileIndex++)
    {
        uint lightIndex = Resources::tileIndexList[bufferOffset + lightTileIndex];
        Lighting::Data light = Lighting::list[lightIndex];

        [branch]
        if (lightIndex == Lighting::maximumListSize)
        {
            break;
        }

        float3 lightRay = light.position - surfacePosition;
        float3 centerToRay = dot(lightRay, reflectNormal) * reflectNormal - lightRay;
        float3 closestPoint = lightRay + centerToRay * clamp(light.radius / length(centerToRay), 0.0, 1.0);
        float lightDistanceSquared = dot(closestPoint, closestPoint);
        float lightDistance = sqrt(lightDistanceSquared);
        float3 lightDirection = (closestPoint / lightDistance);

        float attenuation = 1.0;

        float NdotL = dot(surfaceNormal, lightDirection);

        float3 lightContribution = getBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL);
        surfaceColor += (saturate(NdotL) * lightContribution * attenuation * light.color);
    }

    return surfaceColor;
}

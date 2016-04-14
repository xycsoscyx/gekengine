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
    const uint bufferOffset = (tileIndex * (Lighting::maximumListSize + 1));

    float3 surfaceColor = 0.0;

    uint lightCount = Resources::tileIndexList[bufferOffset];

    [loop]
    for (uint lightTileIndex = 1; lightTileIndex <= lightCount; ++lightTileIndex)
    {
        uint lightIndex = Resources::tileIndexList[bufferOffset + lightTileIndex];
        Lighting::Data light = Lighting::list[lightIndex];

        float3 lightRay = (light.transform._m30_m31_m32 - surfacePosition);
        float3 centerToRay = ((dot(lightRay, reflectNormal) * reflectNormal) - lightRay);
        float3 closestPoint = (lightRay + (centerToRay * clamp((light.radius / length(centerToRay)), 0.0, 1.0)));
        float3 lightDirection = normalize(closestPoint);
        float lightDistance = length(closestPoint);
        //return light.transform._m30_m31_m32;

        float distanceOverRange = (lightDistance / light.range);
        float distanceOverRange2 = sqr(distanceOverRange);
        float distanceOverRange4 = sqr(distanceOverRange2);
        float falloff = sqr(saturate(1.0 - distanceOverRange4));
        falloff /= (sqr(lightDistance) + 1.0);

        float NdotL = dot(surfaceNormal, lightDirection);

        float3 diffuseAlbedo = lerp(materialAlbedo, 0.0, materialMetalness);
        float3 diffuseLighting = (diffuseAlbedo * Math::ReciprocalPi);

        float3 specularLighting = getSpecularBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, NdotL);
        surfaceColor += (saturate(NdotL) * (diffuseLighting + specularLighting) * falloff * light.color);
    }

    return surfaceColor;
}

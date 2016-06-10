#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

#include "BRDF.Custom.h"

struct LightProperties
{
    float falloff;
    float3 direction;
};

LightProperties getPointLightProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
{
    LightProperties lightProperties;

    float3 lightRay = (light.position.xyz - surfacePosition);
    float3 centerToRay = ((dot(lightRay, reflectNormal) * reflectNormal) - lightRay);
    float3 closestPoint = (lightRay + (centerToRay * clamp((light.radius / length(centerToRay)), 0.0, 1.0)));
    lightProperties.direction = normalize(closestPoint);
    float lightDistance = length(closestPoint);

    float distanceOverRange = (lightDistance / light.range);
    float distanceOverRange2 = sqr(distanceOverRange);
    float distanceOverRange4 = sqr(distanceOverRange2);
    lightProperties.falloff = sqr(saturate(1.0 - distanceOverRange4));
    lightProperties.falloff /= (sqr(lightDistance) + 1.0);

    return lightProperties;
}

LightProperties getSpotLightProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
{
    LightProperties lightProperties;

    lightProperties.direction = 0.0;
    lightProperties.falloff = 0.0;

    return lightProperties;
}

LightProperties getDirectionalLightProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
{
    LightProperties lightProperties;

    lightProperties.direction = light.direction;
    lightProperties.falloff = 1.0;

    return lightProperties;
}

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
    const uint bufferOffset = (tileIndex * (Lighting::lightsPerPass + 1));
    uint lightTileCount = Resources::tileIndexList[bufferOffset];
    uint lightTileStart = (bufferOffset + 1);
    uint lightTileEnd = (lightTileStart + lightTileCount);

    float3 surfaceColor = 0.0;

    [loop]
    for (uint lightTileIndex = lightTileStart; lightTileIndex < lightTileEnd; ++lightTileIndex)
    {
        uint lightIndex = Resources::tileIndexList[lightTileIndex];
        Lighting::Data light = Lighting::list[lightIndex];

        LightProperties lightProperties;
        switch (light.type)
        {
        case Lighting::Type::Point:
            lightProperties = getPointLightProperties(light, surfacePosition, surfaceNormal, reflectNormal);
            break;

        case Lighting::Type::Spot:
            lightProperties = getSpotLightProperties(light, surfacePosition, surfaceNormal, reflectNormal);
            break;

        case Lighting::Type::Directional:
            lightProperties = getDirectionalLightProperties(light, surfacePosition, surfaceNormal, reflectNormal);
            break;
        };

        float NdotL = dot(surfaceNormal, lightProperties.direction);
        float3 diffuseAlbedo = lerp(materialAlbedo, 0.0, materialMetalness);
        float3 diffuseLighting = (diffuseAlbedo * Math::ReciprocalPi);
        float3 specularLighting = getSpecularBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightProperties.direction, viewDirection, NdotL);
        surfaceColor += (saturate(NdotL) * (diffuseLighting + specularLighting) * lightProperties.falloff * light.color);
    }

    return surfaceColor;
}

#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

#include "BRDF.Custom.h"

namespace Light
{
    struct Properties
    {
        float falloff;
        float3 direction;
    };

    namespace Punctual
    {
        Properties getPointProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties;

            float3 lightRay = (light.position.xyz - surfacePosition);
            float lightDistance = length(lightRay);
            properties.direction = (lightRay / lightDistance);

            float distanceOverRange = (lightDistance / light.range);
            float distanceOverRange2 = square(distanceOverRange);
            float distanceOverRange4 = square(distanceOverRange2);
            properties.falloff = square(saturate(1.0 - distanceOverRange4));
            properties.falloff /= (square(lightDistance) + 1.0);

            return properties;
        }

        Properties getDirectionalProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties;

            properties.direction = light.direction;
            properties.falloff = 1.0;

            return properties;
        }

        Properties getSpotProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties;

            float3 lightRay = (light.position.xyz - surfacePosition);
            float lightDistance = length(lightRay);
            properties.direction = (lightRay / lightDistance);

            float distanceOverRange = (lightDistance / light.range);
            float distanceOverRange2 = square(distanceOverRange);
            float distanceOverRange4 = square(distanceOverRange2);
            properties.falloff = square(saturate(1.0 - distanceOverRange4));
            properties.falloff /= (square(lightDistance) + 1.0);

            float rho = saturate(dot(light.direction, -properties.direction));
            float spotFactor = pow(saturate(rho - light.outerAngle) / (light.innerAngle - light.outerAngle), light.falloff);
            properties.falloff *= spotFactor;

            return properties;
        }
    };

    namespace Area
    {
        Properties getPointProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties;

            float3 lightRay = (light.position.xyz - surfacePosition);
            float3 centerToRay = ((dot(lightRay, reflectNormal) * reflectNormal) - lightRay);
            float3 closestPoint = (lightRay + (centerToRay * clamp((light.radius / length(centerToRay)), 0.0, 1.0)));
            properties.direction = normalize(closestPoint);
            float lightDistance = length(closestPoint);

            float distanceOverRange = (lightDistance / light.range);
            float distanceOverRange2 = square(distanceOverRange);
            float distanceOverRange4 = square(distanceOverRange2);
            properties.falloff = square(saturate(1.0 - distanceOverRange4));
            properties.falloff /= (square(lightDistance) + 1.0);

            return properties;
        }
    };

    Properties getProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
    {
        [branch]
        switch (light.type)
        {
        case Lighting::Type::Point:
            return Area::getPointProperties(light, surfacePosition, surfaceNormal, reflectNormal);

        case Lighting::Type::Directional:
            return Punctual::getDirectionalProperties(light, surfacePosition, surfaceNormal, reflectNormal);

        case Lighting::Type::Spot:
            return Punctual::getSpotProperties(light, surfacePosition, surfaceNormal, reflectNormal);
        };

        return (Properties)0;
    }
};

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float3 materialAlbedo = Resources::albedoBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float2 materialInfo = Resources::materialBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float materialRoughness = ((materialInfo.x * 0.9) + 0.1); // account for infinitely small point lights
    float materialMetalness = materialInfo.y;

    float sceneDepth = Resources::depth.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float3 surfacePosition = getViewPosition(inputPixel.texCoord, sceneDepth);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0));

    float3 viewDirection = -normalize(surfacePosition);
    float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(Defines::tileSize).xx));
    const uint tileIndex = ((tilePosition.y * Defines::dispatchWidth) + tilePosition.x);
    const uint bufferOffset = (tileIndex * (Lighting::lightsPerPass + 1));
    uint lightTileCount = Resources::tileIndexList[bufferOffset];
    uint lightTileStart = (bufferOffset + 1);
    uint lightTileEnd = (lightTileStart + lightTileCount);

    OutputPixel outputPixel;
    outputPixel.lightAccumulationBuffer = 0.0;

    [loop]
    for (uint lightTileIndex = lightTileStart; lightTileIndex < lightTileEnd; ++lightTileIndex)
    {
        uint lightIndex = Resources::tileIndexList[lightTileIndex];
        Lighting::Data light = Lighting::list[lightIndex];

        Light::Properties lightProperties = Light::getProperties(light, surfacePosition, surfaceNormal, reflectNormal);

        float NdotL = dot(surfaceNormal, lightProperties.direction);
        float3 diffuseAlbedo = lerp(materialAlbedo, 0.0, materialMetalness);
        float3 diffuseLighting = (diffuseAlbedo * Math::ReciprocalPi);
        float3 specularLighting = getSpecularBRDF(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightProperties.direction, viewDirection, NdotL);
        outputPixel.lightAccumulationBuffer += (saturate(NdotL) * (diffuseLighting + specularLighting) * lightProperties.falloff * light.color);
    }

    return outputPixel;
}

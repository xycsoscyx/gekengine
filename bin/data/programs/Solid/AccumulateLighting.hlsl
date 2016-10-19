#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

namespace Light
{
	struct Properties
	{
        float3 contribution;
        float3 direction;
	};

    float getFalloff(in float distance, in float range)
    {
        const float denominator = (pow(distance, 2.0) + 1.0);
        const float attenuation = pow((distance / range), 4.0);
        return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
    }

    Properties getProperties(in Lighting::DirectionalData lightData, in float3 surfacePosition, in float3 surfaceNormal, in float3 reflectNormal)
    {
        Properties properties;
        properties.direction = lightData.direction;
        properties.contribution = lightData.color;
        return properties;
    }

    namespace Punctual
	{
        Properties getProperties(in Lighting::PointData lightData, in float3 surfacePosition, in float3 surfaceNormal, in float3 reflectNormal)
		{
            const float3 lightRay = (lightData.position.xyz - surfacePosition);
			const float lightDistance = length(lightRay);
            
            Properties properties;
            properties.direction = (lightRay / lightDistance);
            properties.contribution = (lightData.color * getFalloff(lightDistance, lightData.range));
            return properties;
		}

		Properties getProperties(in Lighting::SpotData lightData, in float3 surfacePosition, in float3 surfaceNormal, in float3 reflectNormal)
		{
            const float3 lightRay = (lightData.position.xyz - surfacePosition);
            const float lightDistance = length(lightRay);

            Properties properties;
            properties.direction = (lightRay / lightDistance);
            const float rho = saturate(dot(lightData.direction, -properties.direction));
            const float spotFactor = pow(saturate(rho - lightData.outerAngle) / (lightData.innerAngle - lightData.outerAngle), 4.0);
            properties.contribution = (lightData.color * getFalloff(lightDistance, lightData.range) * spotFactor);

            return properties;
		}
	};

	namespace Area
	{
		Properties getProperties(in Lighting::PointData lightData, in float3 surfacePosition, in float3 surfaceNormal, in float3 reflectNormal)
		{
            const float3 lightRay = (lightData.position.xyz - surfacePosition);
            const float3 centerToRay = (lightRay - (dot(lightRay, reflectNormal) * reflectNormal));
            const float3 closestPoint = (lightRay + (centerToRay * saturate(lightData.radius / length(centerToRay))));
            const float lightDistance = length(closestPoint);

            Properties properties;
            properties.direction = normalize(closestPoint);
            properties.contribution = (lightData.color * getFalloff(lightDistance, lightData.range));
			return properties;
		}
    };

    // Normal Distribution Function ( NDF ) or D( h )
    // GGX ( Trowbridge-Reitz )
    float getDistributionGGX(in float HdotN, in float alpha)
    {
        // alpha is assumed to be roughness^2
        const float alphaSquared = pow(alpha, 2.0);

        //float denominator = (HdotN * HdotN) * (alphaSquared - 1.0) + 1.0;
        const float denominator = (HdotN * alphaSquared - HdotN) * HdotN + 1.0;
        return (alphaSquared / (Math::Pi * denominator * denominator));
    }

    float getDistributionDisneyGGX(in float HdotN, in float alpha)
    {
        // alpha is assumed to be roughness^2
        const float alphaSquared = pow(alpha, 2.0);

        const float denominator = (HdotN * HdotN) * (alphaSquared - 1.0) + 1.0;
        return (alphaSquared / (Math::Pi * denominator));
    }

    float getDistribution1886GGX(in float HdotN, in float alpha)
    {
        return (alpha / (Math::Pi * pow(HdotN * HdotN * (alpha - 1.0) + 1.0, 2.0)));
    }

    // Visibility term G( l, v, h )
    // Very similar to Marmoset Toolbag 2 and gives almost the same results as Smith GGX
    float getVisibilitySchlick(in float VdotN, in float LdotN, in float alpha)
    {
        const float k = alpha * 0.5;
        const float schlickL = (LdotN * (1.0 - k) + k);
        const float schlickV = (VdotN * (1.0 - k) + k);
        return (0.25 / (schlickL * schlickV));
        //return ((schlickL * schlickV) / (4.0 * VdotN * LdotN));
    }

    // see s2013_pbs_rad_notes.pdf
    // Crafting a Next-Gen Material Pipeline for The Order: 1886
    // this visibility function also provides some sort of back lighting
    float getVisibilitySmithGGX(in float VdotN, in float LdotN, in float alpha)
    {
        const float V1 = LdotN + sqrt(alpha + (1.0 - alpha) * LdotN * LdotN);
        const float V2 = VdotN + sqrt(alpha + (1.0 - alpha) * VdotN * VdotN);

        // avoid too bright spots
        return (1.0 / max(V1 * V2, 0.15));
        //return (V1 * V2);
    }

    // Fresnel term F( v, h )
    // Fnone( v, h ) = F(0?) = specularColor
    float3 getFresnelSchlick(in float3 specularColor, in float VdotH)
    {
        return specularColor + (1.0 - specularColor) * pow(1.0 - VdotH, 5.0);
    }

    float3 getContribution(in Light::Properties lightProperties, in float3 surfaceNormal, in float3 viewDirection, in float3 VdotN, in float3 materialAlbedo, in float materialRoughness, in float materialMetallic, in float alpha, in float alphaG)
    {
        const half LdotN = saturate(dot(surfaceNormal, lightProperties.direction));

        half lambert;
        if (Defines::useHalfLambert)
        {
            // http://developer.valvesoftware.com/wiki/Half_Lambert
            half halfLdotN = dot(surfaceNormal, lightProperties.direction) * 0.5 + 0.5;
            lambert = pow(halfLdotN, 2.0);
        }
        else
        {
            lambert = LdotN;
        }

        const half3 halfAngleVector = normalize(lightProperties.direction + viewDirection);
        const half HdotN = saturate(dot(halfAngleVector, surfaceNormal));

        const half VdotH = saturate(dot(viewDirection, halfAngleVector));

        const half3 reflectColor = lerp(materialAlbedo, lightProperties.contribution, materialMetallic);

        //const half3 D = pow(abs(HdotN), 10.0f);
        const half3 D = getDistributionGGX(HdotN, alpha);
        //const half3 D = getDistribution1886GGX(HdotN, alpha);
        //const half3 D = getDistributionDisneyGGX(HdotN, alphaG);
        const half3 G = getVisibilitySchlick(LdotN, VdotN, alpha);
        //const half3 G = getVisibilitySmithGGX(LdotN, VdotN, alpha);
        const half3 F = getFresnelSchlick(reflectColor, VdotH);

        // horizon
        const float horizon = pow((1.0 - LdotN), 4.0);
        const half3 specularLightColor = lightProperties.contribution - lightProperties.contribution * horizon;
        const float3 specularColor = saturate(D * G * (F * (specularLightColor * lambert)));

        // see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        lambert /= Math::Pi;
        half3 diffuseColor = lerp(materialAlbedo, 0.0, materialMetallic) * lightProperties.contribution * lambert;

        /* Maintain energy conservation
            Energy conservation is a restriction on the reflection model
            that requires that the total amount of reflected light
            cannot be more than the incoming light.
            http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
        */
        diffuseColor *= (1.0 - specularColor);
        return (diffuseColor + specularColor);
    }
};

float3 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    const float3 materialAlbedo = Resources::albedoBuffer[inputPixel.screen.xy];
	const float2 materialInfo = Resources::materialBuffer[inputPixel.screen.xy];
    const float materialRoughness = ((materialInfo.x * 0.9) + 0.1); // account for infinitely small point lights
    const float materialMetallic = materialInfo.y;

    const float surfaceDepth = Resources::depthBuffer[inputPixel.screen.xy];
    const float3 surfacePosition = getPositionFromSample(inputPixel.texCoord, surfaceDepth);
    const float3 surfaceNormal = getDecodedNormal(Resources::normalBuffer[inputPixel.screen.xy]);

    const float3 viewDirection = -normalize(surfacePosition);
    const float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

    const half VdotN = saturate(dot(viewDirection, surfaceNormal));

    // alpha modifications by Disney - s2012_pbs_disney_brdf_notes_v2.pdf
    const half alpha = pow(materialRoughness, 2.0);

    // reduce roughness range from [0 .. 1] to [0.5 .. 1]
    const half alphaG = pow(0.5 + materialRoughness * 0.5, 2.0);

    float3 surfaceLight = 0;

    [loop]
    for (uint directionalIndex = 0; directionalIndex < Lighting::directionalCount; directionalIndex++)
    {
        const Lighting::DirectionalData light = Lighting::directionalList[directionalIndex];
        const Light::Properties lightProperties = Light::getProperties(light, surfacePosition, surfaceNormal, reflectNormal);
        surfaceLight += Light::getContribution(lightProperties, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, alphaG);
    }

    [loop]
    for (uint pointIndex = 0; pointIndex < Lighting::pointCount; pointIndex++)
    {
        const Lighting::PointData light = Lighting::pointList[pointIndex];
        const Light::Properties lightProperties = Light::Area::getProperties(light, surfacePosition, surfaceNormal, reflectNormal);
        surfaceLight += Light::getContribution(lightProperties, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, alphaG);
    }

    [loop]
    for (uint spotIndex = 0; spotIndex < Lighting::spotCount; spotIndex++)
    {
        const Lighting::SpotData light = Lighting::spotList[spotIndex];
        const Light::Properties lightProperties = Light::Punctual::getProperties(light, surfacePosition, surfaceNormal, reflectNormal);
        surfaceLight += Light::getContribution(lightProperties, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, alphaG);
    }

    return surfaceLight;
}

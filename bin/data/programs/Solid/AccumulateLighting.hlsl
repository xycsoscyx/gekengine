#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

namespace Light
{
	struct Properties
	{
		float falloff;
		float3 direction;
	};

    float getFalloff(float distance, float range)
    {
        float denominator = (pow(distance, 2.0) + 1.0);
        float attenuation = pow((distance / range), 4.0);
        return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
    }

	namespace Punctual
	{
        Properties getDirectionalProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties;

            properties.direction = light.direction;

            properties.falloff = 1.0;

            return properties;
        }

        Properties getPointProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
		{
			Properties properties;

			float3 lightRay = (light.position.xyz - surfacePosition);
			float lightDistance = length(lightRay);
			properties.direction = (lightRay / lightDistance);

            properties.falloff = getFalloff(lightDistance, light.range);

			return properties;
		}

		Properties getSpotProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
		{
            Properties properties = getPointProperties(light, surfacePosition, surfaceNormal, reflectNormal);

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
			float3 centerToRay = (lightRay - (dot(lightRay, reflectNormal) * reflectNormal));
            float3 closestPoint = (lightRay + (centerToRay * saturate(light.radius / length(centerToRay))));
			properties.direction = normalize(closestPoint);

            float lightDistance = length(closestPoint);
            properties.falloff = getFalloff(lightDistance, light.range);

			return properties;
		}

        Properties getSpotProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
        {
            Properties properties = getPointProperties(light, surfacePosition, surfaceNormal, reflectNormal);

            float rho = saturate(dot(light.direction, -properties.direction));
            float spotFactor = pow(saturate(rho - light.outerAngle) / (light.innerAngle - light.outerAngle), light.falloff);
            properties.falloff *= spotFactor;

            return properties;
        }
    };

	Properties getProperties(Lighting::Data light, float3 surfacePosition, float3 surfaceNormal, float3 reflectNormal)
	{
		[branch]
		switch (light.type)
		{
        case Lighting::Type::Directional:
            return Punctual::getDirectionalProperties(light, surfacePosition, surfaceNormal, reflectNormal);

        case Lighting::Type::Point:
			return Area::getPointProperties(light, surfacePosition, surfaceNormal, reflectNormal);

		case Lighting::Type::Spot:
			return Punctual::getSpotProperties(light, surfacePosition, surfaceNormal, reflectNormal);
		};

		return (Properties)0;
	}
};

// Normal Distribution Function ( NDF ) or D( h )
// GGX ( Trowbridge-Reitz )
float getDistributionGGX(float HdotN, float alpha)
{
	// alpha is assumed to be roughness^2
	float alphaSquared = pow(alpha, 2.0);

	//float denominator = (HdotN * HdotN) * (alphaSquared - 1.0) + 1.0;
	float denominator = (HdotN * alphaSquared - HdotN) * HdotN + 1.0;
	return (alphaSquared / (Math::Pi * denominator * denominator));
}

float getDistributionDisneyGGX(float HdotN, float alpha)
{
	// alpha is assumed to be roughness^2
    float alphaSquared = pow(alpha, 2.0);

	float denominator = (HdotN * HdotN) * (alphaSquared - 1.0) + 1.0;
	return (alphaSquared / (Math::Pi * denominator));
}

float getDistribution1886GGX(float HdotN, float alpha)
{
	return (alpha / (Math::Pi * pow(HdotN * HdotN * (alpha - 1.0) + 1.0, 2.0)));
}

// Visibility term G( l, v, h )
// Very similar to Marmoset Toolbag 2 and gives almost the same results as Smith GGX
float getVisibilitySchlick(float VdotN, float LdotN, float alpha)
{
	float k = alpha * 0.5;
	float schlickL = (LdotN * (1.0 - k) + k);
	float schlickV = (VdotN * (1.0 - k) + k);
	return (0.25 / (schlickL * schlickV));
	//return ((schlickL * schlickV) / (4.0 * VdotN * LdotN));
}

// see s2013_pbs_rad_notes.pdf
// Crafting a Next-Gen Material Pipeline for The Order: 1886
// this visibility function also provides some sort of back lighting
float getVisibilitySmithGGX(float VdotN, float LdotN, float alpha)
{
	float V1 = LdotN + sqrt(alpha + (1.0 - alpha) * LdotN * LdotN);
	float V2 = VdotN + sqrt(alpha + (1.0 - alpha) * VdotN * VdotN);
	
	// avoid too bright spots
	return (1.0 / max(V1 * V2, 0.15));
	//return (V1 * V2);
}

// Fresnel term F( v, h )
// Fnone( v, h ) = F(0?) = specularColor
float3 getFresnelSchlick(float3 specularColor, float VdotH)
{
	return specularColor + (1.0 - specularColor) * pow(1.0 - VdotH, 5.0);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
	float3 materialAlbedo = Resources::albedoBuffer[inputPixel.screen.xy];
	float2 materialInfo = Resources::materialBuffer[inputPixel.screen.xy];
	float materialRoughness = ((materialInfo.x * 0.9) + 0.1); // account for infinitely small point lights
	float materialMetallic = materialInfo.y;

	float surfaceDepth = Resources::depthBuffer[inputPixel.screen.xy];
	float3 surfacePosition = getPositionFromSample(inputPixel.texCoord, surfaceDepth);
	float3 surfaceNormal = getDecodedNormal(Resources::normalBuffer[inputPixel.screen.xy]);

	float3 viewDirection = -normalize(surfacePosition);
	float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

	const uint2 tilePosition = uint2(floor(inputPixel.screen.xy / float(Defines::tileSize).xx));
	const uint tileIndex = ((tilePosition.y * Defines::dispatchWidth) + tilePosition.x);
	const uint bufferOffset = (tileIndex * (Lighting::lightsPerPass + 1));
	uint lightTileCount = Resources::tileIndexList[bufferOffset];
	uint lightTileStart = (bufferOffset + 1);
	uint lightTileEnd = (lightTileStart + lightTileCount);

	float3 surfaceLight = 0.0;

	[loop]
	for (uint lightTileIndex = lightTileStart; lightTileIndex < lightTileEnd; ++lightTileIndex)
	{
		uint lightIndex = Resources::tileIndexList[lightTileIndex];
		Lighting::Data light = Lighting::list[lightIndex];

		Light::Properties lightProperties = Light::getProperties(light, surfacePosition, surfaceNormal, reflectNormal);

		half LdotN = saturate(dot(surfaceNormal, lightProperties.direction));

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

		half3 halfAngleVector = normalize(lightProperties.direction + viewDirection);
		half HdotN = saturate(dot(halfAngleVector, surfaceNormal));

		half VdotN = saturate(dot(viewDirection, surfaceNormal));
		half VdotH = saturate(dot(viewDirection, halfAngleVector));

		half3 lightColor = lightProperties.falloff * light.color;
		half3 reflectColor = lerp(materialAlbedo, lightColor, materialMetallic);

		// alpha modifications by Disney - s2012_pbs_disney_brdf_notes_v2.pdf
		const half alpha = pow(materialRoughness, 2.0);

		// reduce roughness range from [0 .. 1] to [0.5 .. 1]
		const half alphaG = pow(0.5 + materialRoughness * 0.5, 2.0);

		//half3 D = pow(abs(HdotN), 10.0f);
		half3 D = getDistributionGGX(HdotN, alpha);
		//half3 D = getDistribution1886GGX(HdotN, alpha);
		//half3 D = getDistributionDisneyGGX(HdotN, alphaG);
		half3 G = getVisibilitySchlick(LdotN, VdotN, alpha);
		//half3 G = getVisibilitySmithGGX(LdotN, VdotN, alpha);
		half3 F = getFresnelSchlick(reflectColor, VdotH);

		// horizon
        float horizon = pow((1.0 - LdotN), 4.0);
		half3 specularLightColor = lightColor - lightColor * horizon;
		float3 specularColor = saturate(D * G * (F * (specularLightColor * lambert)));

		// see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		lambert /= Math::Pi;
		half3 diffuseColor = lerp(materialAlbedo, 0.0, materialMetallic) * lightColor * lambert;

		/* Maintain energy conservation
			Energy conservation is a restriction on the reflection model
			that requires that the total amount of reflected light
			cannot be more than the incoming light.
			http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
		*/
		diffuseColor *= (1.0 - specularColor);
		surfaceLight += (diffuseColor + specularColor);
	}

	return surfaceLight;
}

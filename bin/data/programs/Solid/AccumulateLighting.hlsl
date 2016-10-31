#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float getFalloff(float distance, float range)
{
    float denominator = (pow(distance, 2.0) + 1.0);
    float attenuation = pow((distance / range), 4.0);
    return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
}

// Normal Distribution Function ( NDF ) or D( h )
// GGX ( Trowbridge-Reitz )
float getDistributionGGX(float HdotN, float alpha)
{
    // alpha is assumed to be roughness^2
    float alphaSquared = pow(alpha, 2.0);

    //float denominator = (HdotN * HdotN) * (alphaSquared - 1.0) + 1.0;
    float denominator = ((((HdotN * alphaSquared) - HdotN) * HdotN) + 1.0);
    return (alphaSquared / (Math::Pi * denominator * denominator));
}

float getDistributionDisneyGGX(float HdotN, float alpha)
{
    // alpha is assumed to be roughness^2
    float alphaSquared = pow(alpha, 2.0);

    float denominator = (((HdotN * HdotN) * (alphaSquared - 1.0)) + 1.0);
    return (alphaSquared / (Math::Pi * denominator));
}

float getDistribution1886GGX(float HdotN, float alpha)
{
    return (alpha / (Math::Pi * pow(((HdotN * HdotN * (alpha - 1.0)) + 1.0), 2.0)));
}

// Visibility term G( l, v, h )
// Very similar to Marmoset Toolbag 2 and gives almost the same results as Smith GGX
float getVisibilitySchlick(float VdotN, float LdotN, float alpha)
{
    float k = (alpha * 0.5);
    float schlickL = ((LdotN * (1.0 - k)) + k);
    float schlickV = ((VdotN * (1.0 - k)) + k);
    return (0.25 / (schlickL * schlickV));
    //return ((schlickL * schlickV) / (4.0 * VdotN * LdotN));
}

// see s2013_pbs_rad_notes.pdf
// Crafting a Next-Gen Material Pipeline for The Order: 1886
// this visibility function also provides some sort of back lighting
float getVisibilitySmithGGX(float VdotN, float LdotN, float alpha)
{
    float V1 = (LdotN + (sqrt(alpha + ((1.0 - alpha) * LdotN * LdotN))));
    float V2 = (VdotN + (sqrt(alpha + ((1.0 - alpha) * VdotN * VdotN))));

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

float3 getSurfaceIrradiance(
	float3 lightDirection, float3 lightRadiance, 
	float3 surfaceNormal, 
	float3 viewDirection, float VdotN,
	float3 materialAlbedo, float materialRoughness, float materialMetallic,  
	float alpha, float clampedAlpha)
{
    float LdotN = saturate(dot(surfaceNormal, lightDirection));

    float lambert;
    if (Defines::useHalfLambert)
    {
        // http://developer.valvesoftware.com/wiki/Half_Lambert
        float halfLdotN = ((dot(surfaceNormal, lightDirection) * 0.5) + 0.5);
        lambert = pow(halfLdotN, 2.0);
    }
    else
    {
        lambert = LdotN;
    }

    float3 halfAngleVector = normalize(lightDirection + viewDirection);
    float HdotN = saturate(dot(halfAngleVector, surfaceNormal));

    float VdotH = saturate(dot(viewDirection, halfAngleVector));

    float3 reflectColor = lerp(materialAlbedo, lightRadiance, materialMetallic);

    //float D = pow(abs(HdotN), 10.0f);
    float D = getDistributionGGX(HdotN, alpha);
    //float D = getDistribution1886GGX(HdotN, alpha);
    //float D = getDistributionDisneyGGX(HdotN, clampedAlpha);
    float G = getVisibilitySchlick(LdotN, VdotN, alpha);
    //float G = getVisibilitySmithGGX(LdotN, VdotN, alpha);
    float3 F = getFresnelSchlick(reflectColor, VdotH);

    // horizon
    float horizon = pow((1.0 - LdotN), 4.0);
    float3 specularLightColor = (lightRadiance - (lightRadiance * horizon));
    float3 specularColor = saturate(D * G * (F * (specularLightColor * lambert)));

    // see http://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    lambert /= Math::Pi;
    float3 diffuseColor = saturate(lerp(materialAlbedo, 0.0, materialMetallic) * lightRadiance * lambert);

    /* Maintain energy conservation
        Energy conservation is a restriction on the reflection model
        that requires that the total amount of reflected light
        cannot be more than the incoming light.
        http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
    */
    diffuseColor *= (1.0 - specularColor);

    return (diffuseColor + specularColor);
}

uint3 getClusterLocation(float2 screenPosition, float surfaceDepth)
{
	float2 screenCoord = (screenPosition * Shader::pixelSize);
	uint2 gridLocation = floor(screenCoord * float2(16.0, 8.0));

	float depth = (surfaceDepth - Camera::nearClip) / (Camera::farClip - Camera::nearClip);
	uint gridDepth = floor(depth * 24.0);

	return uint3(gridLocation, gridDepth);
}

uint getClusterOffset(int3 clusterLocation)
{
	return ((((clusterLocation.z * 8) + clusterLocation.y) * 16) + clusterLocation.x) * 3;
}

uint getClusterOffset(float2 screenPosition, float surfaceDepth)
{
	return getClusterOffset(getClusterLocation(screenPosition, surfaceDepth));
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer[inputPixel.screen.xy];
	float2 materialInfo = Resources::materialBuffer[inputPixel.screen.xy];
    float materialRoughness = ((materialInfo.x * 0.9) + 0.1); // account for infinitely small point lights
    float materialMetallic = materialInfo.y;

    float surfaceAmbient = Resources::ambientBuffer[inputPixel.screen.xy];
    float surfaceDepth = Resources::depthBuffer[inputPixel.screen.xy];
    float3 surfacePosition = getPositionFromSample(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = getDecodedNormal(Resources::normalBuffer[inputPixel.screen.xy]);

    float3 viewDirection = -normalize(surfacePosition);
    float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

    float VdotN = saturate(dot(viewDirection, surfaceNormal));

    // alpha modifications by Disney - s2012_pbs_disney_brdf_notes_v2.pdf
    float alpha = pow(materialRoughness, 2.0);

    // reduce roughness range from [0 .. 1] to [0.5 .. 1]
    float clampedAlpha = pow(0.5 + materialRoughness * 0.5, 2.0);

	uint clusterOffset = getClusterOffset(inputPixel.screen.xy, surfacePosition.z);
	uint indexOffset = Lights::clusterDataList[clusterOffset + 0];
	uint pointLightCount = Lights::clusterDataList[clusterOffset + 1];
	uint spotLightCount = Lights::clusterDataList[clusterOffset + 2];

	float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		float3 lightDirection = Lights::directionalList[directionalIndex].direction;
		float3 lightRadiance = Lights::directionalList[directionalIndex].color;
		surfaceIrradiance += getSurfaceIrradiance(lightDirection, lightRadiance, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, clampedAlpha);
	}

	while (pointLightCount-- > 0)
	{
		uint lightIndex = Lights::clusterIndexList[indexOffset++];
		Lights::PointData lightData = Lights::pointList[lightIndex];

		float3 lightRay = (lightData.position.xyz - surfacePosition);
		float3 centerToRay = (lightRay - (dot(lightRay, reflectNormal) * reflectNormal));
		float3 closestPoint = (lightRay + (centerToRay * saturate(lightData.radius / length(centerToRay))));
		float lightDistance = length(closestPoint);
		float3 lightDirection = normalize(closestPoint);
		float3 lightRadiance = (lightData.color * getFalloff(lightDistance, lightData.range));

		surfaceIrradiance += getSurfaceIrradiance(lightDirection, lightRadiance, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, clampedAlpha);
	};

	while (spotLightCount-- > 0)
	{
		uint lightIndex = Lights::clusterIndexList[indexOffset++];
		Lights::SpotData lightData = Lights::spotList[lightIndex];

		float3 lightRay = (lightData.position.xyz - surfacePosition);
		float lightDistance = length(lightRay);
		float3 lightDirection = (lightRay / lightDistance);
		float rho = saturate(dot(lightData.direction, -lightDirection));
		float spotFactor = pow(saturate(rho - lightData.outerAngle) / (lightData.innerAngle - lightData.outerAngle), lightData.coneFalloff);
		float3 lightRadiance = (lightData.color * getFalloff(lightDistance, lightData.range) * spotFactor);

		surfaceIrradiance += getSurfaceIrradiance(lightDirection, lightRadiance, surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, alpha, clampedAlpha);
	};

	return (surfaceIrradiance + (materialAlbedo * surfaceAmbient * 0.005));
}

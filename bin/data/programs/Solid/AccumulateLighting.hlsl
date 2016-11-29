#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord);

    [branch]
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);

    float3 surfaceNormal;
    // assume normals are stored as 3Dc format, so generate the Z value
    surfaceNormal.xy = Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    surfaceNormal.xy = ((surfaceNormal.xy * 2.0) - 1.0);
    surfaceNormal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    surfaceNormal.z = sqrt(1.0 - dot(surfaceNormal.xy, surfaceNormal.xy));
    surfaceNormal = mul(surfaceNormal, viewBasis);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::linearWrapSampler, inputPixel.texCoord);

    float3 viewDirection = -normalize(surfacePosition);
    float3 reflectNormal = reflect(-viewDirection, surfaceNormal);

    float VdotN = saturate(dot(viewDirection, surfaceNormal));

    // materialAlpha modifications by Disney - s2012_pbs_disney_brdf_notes_v2.pdf
    float materialAlpha = pow(materialRoughness, 2.0);

    // reduce roughness range from [0 .. 1] to [0.5 .. 1]
    float materialDisneyAlpha = pow(0.5 + materialRoughness * 0.5, 2.0);

	float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		float3 lightDirection = Lights::directionalList[directionalIndex].direction;
		float3 lightRadiance = Lights::directionalList[directionalIndex].radiance;

		surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, materialAlpha, materialDisneyAlpha, lightDirection, lightRadiance);
	}

	uint clusterOffset = getClusterOffset(inputPixel.screen.xy, surfacePosition.z);
	uint3 clusterData = Lights::clusterDataList[clusterOffset];
	uint indexOffset = clusterData.x;
	uint pointLightCount = clusterData.y;
	uint spotLightCount = clusterData.z;

	while (pointLightCount-- > 0)
	{
		uint lightIndex = Lights::clusterIndexList[indexOffset++];
		Lights::PointData lightData = Lights::pointList[lightIndex];

		float3 lightRay = (lightData.position - surfacePosition);
		float3 centerToRay = (lightRay - (dot(lightRay, reflectNormal) * reflectNormal));
		float3 closestPoint = (lightRay + (centerToRay * saturate(lightData.radius / length(centerToRay))));
		float lightDistance = length(closestPoint);
		float3 lightDirection = normalize(closestPoint);
		float3 lightRadiance = (lightData.radiance * getFalloff(lightDistance, lightData.range));

        surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, materialAlpha, materialDisneyAlpha, lightDirection, lightRadiance);
	};

	while (spotLightCount-- > 0)
	{
		uint lightIndex = Lights::clusterIndexList[indexOffset++];
		Lights::SpotData lightData = Lights::spotList[lightIndex];

		float3 lightRay = (lightData.position - surfacePosition);
		float lightDistance = length(lightRay);
		float3 lightDirection = (lightRay / lightDistance);
		float rho = saturate(dot(lightData.direction, -lightDirection));
		float spotFactor = pow(saturate(rho - lightData.outerAngle) / (lightData.innerAngle - lightData.outerAngle), lightData.coneFalloff);
		float3 lightRadiance = (lightData.radiance * getFalloff(lightDistance, lightData.range) * spotFactor);

		surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, VdotN, materialAlbedo, materialRoughness, materialMetallic, materialAlpha, materialDisneyAlpha, lightDirection, lightRadiance);
	};

    OutputPixel outputPixel;
    outputPixel.screen = surfaceIrradiance;
    outputPixel.normalBuffer = getEncodedNormal(surfaceNormal);
    return outputPixel;
}

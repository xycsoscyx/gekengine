#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

float3 getGlassAverage(float2 texCoord, int glassLevel)
{
    //--------------------------------------------------------------------------------------
    // Calculate the center of the texel to avoid any filtering

    uint levels;
    float2 textureDimensions;
    Resources::glassBuffer.GetDimensions(glassLevel, textureDimensions.x, textureDimensions.y, levels);
    float2 invTextureDimensions = 1.f / textureDimensions;

    texCoord *= textureDimensions;

    float2 texelCenter = floor(texCoord - 0.5f) + 0.5f;
    float2 fracOffset = texCoord - texelCenter;
    float2 fracOffset_x2 = fracOffset * fracOffset;
    float2 fracOffset_x3 = fracOffset * fracOffset_x2;

    //--------------------------------------------------------------------------------------
    // Calculate the filter weights (B-Spline Weighting Function)

    float2 weight0 = fracOffset_x2 - 0.5f * (fracOffset_x3 + fracOffset);
    float2 weight1 = 1.5f * fracOffset_x3 - 2.5f * fracOffset_x2 + 1.f;
    float2 weight3 = 0.5f * (fracOffset_x3 - fracOffset_x2);
    float2 weight2 = 1.f - weight0 - weight1 - weight3;

    //--------------------------------------------------------------------------------------
    // Calculate the texture coordinates

    float2 scalingFactor0 = weight0 + weight1;
    float2 scalingFactor1 = weight2 + weight3;

    float2 f0 = weight1 / (weight0 + weight1);
    float2 f1 = weight3 / (weight2 + weight3);

    float2 texCoord0 = texelCenter - 1.f + f0;
    float2 texCoord1 = texelCenter + 1.f + f1;

    texCoord0 *= invTextureDimensions;
    texCoord1 *= invTextureDimensions;

    //--------------------------------------------------------------------------------------
    // Sample the texture

    return
        Resources::glassBuffer.SampleLevel(Global::linearClampSampler, float2(texCoord0.x, texCoord0.y), glassLevel) * scalingFactor0.x * scalingFactor0.y +
        Resources::glassBuffer.SampleLevel(Global::linearClampSampler, float2(texCoord1.x, texCoord0.y), glassLevel) * scalingFactor1.x * scalingFactor0.y +
        Resources::glassBuffer.SampleLevel(Global::linearClampSampler, float2(texCoord0.x, texCoord1.y), glassLevel) * scalingFactor0.x * scalingFactor1.y +
        Resources::glassBuffer.SampleLevel(Global::linearClampSampler, float2(texCoord1.x, texCoord1.y), glassLevel) * scalingFactor1.x * scalingFactor1.y;
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord);

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

    float glassRoughness = (materialRoughness * 5.0);
    int glassLevel = int(floor(glassRoughness));
    float3 glassColor = getGlassAverage((inputPixel.screen.xy * Shader::pixelSize), glassLevel);

    [branch]
    if (materialRoughness < 1.0)
    {
        float lerpLevel = frac(glassRoughness);
        glassColor = lerp(glassColor, getGlassAverage((inputPixel.screen.xy * Shader::pixelSize), (glassLevel + 1)), lerpLevel);
    }

    materialRoughness = 0.1;
    materialMetallic = 1.0;

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

    return surfaceIrradiance + glassColor * materialAlbedo;
}

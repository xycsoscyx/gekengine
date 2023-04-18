// http://www.jordanstevenstechart.com/physically-based-rendering

float getFalloff(const float distance, const float range)
{
    float denominator = (pow(distance, 2.0) + 1.0);
    float attenuation = pow((distance / range), 4.0);
    return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
}

float getSpotFactor(float3 lightDataDirection, float3 lightDirection, float innerAngle, float outerAngle, float coneFalloff)
{
    float rho = max(0.0, (dot(lightDataDirection, -lightDirection)));
    return pow(max(0.0, (rho - outerAngle) / (innerAngle - outerAngle)), coneFalloff);
}

uint getClusterOffset(const float2 screenPosition, const float surfaceDepth)
{
    uint2 gridLocation = floor(screenPosition * Lights::ReciprocalTileSize.xy);

    float depth = ((surfaceDepth - Camera::NearClip) * Camera::ReciprocalClipDistance);
    uint gridSlice = floor(depth * Lights::gridSize.z);

    return ((((gridSlice * Lights::gridSize.y) + gridLocation.y) * Lights::gridSize.x) + gridLocation.x);
}

float3 getLightIrradiance(
    const float3 materialAlbedo,
    const float3 diffuseContribution,
    const float materialRoughness,
    const float materialAlpha,
    const float materialMetallic,
    const float materialNonMetallic,
    const float3 viewDirection,
    const float3 surfaceNormal,
    const float NdotV,
    const float3 lightDirection,
    const float3 lightRadiance,
    const float attenuation)
{
    float3 lightReflectDirection = reflect(-lightDirection, surfaceNormal);
    float3 halfDirection = normalize(viewDirection + lightDirection);
    float NdotL = saturate(dot(surfaceNormal, lightDirection));
    float NdotH = saturate(dot(surfaceNormal, halfDirection));
    float VdotH = saturate(dot(viewDirection, halfDirection));
    float LdotH = saturate(dot(lightDirection, halfDirection));
    float LdotV = saturate(dot(lightDirection, viewDirection));
    float RdotV = saturate(dot(lightReflectDirection, viewDirection));

    float3 F0 = lerp(1.0, materialAlbedo, materialMetallic);

    float normalDistribution;
    switch (Options::BRDF::NormalDistribution::Selection)
    {
    case Options::BRDF::NormalDistribution::Beckmann: {
        float NdotHSquared = square(NdotH);
        normalDistribution = max(0.000001, (1.0 / (Math::Pi * materialAlpha * NdotHSquared * NdotHSquared)) * exp((NdotHSquared - 1.0) / (materialAlpha * NdotHSquared)));
        break; }

    case Options::BRDF::NormalDistribution::Gaussian: {
        float thetaH = acos(NdotH);
        normalDistribution = exp(-thetaH * thetaH / materialAlpha);
        break; }

    case Options::BRDF::NormalDistribution::GGX: {
        float NdotHSquared = square(NdotH);
        float TangentNdotHSquared = (1.0 - NdotHSquared) / NdotHSquared;
        normalDistribution = Math::ReciprocalPi * square(materialRoughness / (NdotHSquared * (materialAlpha + TangentNdotHSquared)));
        break; }

    case Options::BRDF::NormalDistribution::TrowbridgeReitz: {
        float NdotHSquared = square(NdotH);
        float Distribution = (NdotHSquared * (materialAlpha - 1.0) + 1.0);
        normalDistribution = materialAlpha / (Math::Pi * square(Distribution));
        break; }

    default:
        normalDistribution = 0.0;
        break;
    };

    float geometricShadowing;
    switch (Options::BRDF::GeometricShadowing::Selection)
    {
    case Options::BRDF::GeometricShadowing::AshikhminShirley: {
        geometricShadowing = NdotL * NdotV / (LdotH * max(NdotL, NdotV));
        break; }

    case Options::BRDF::GeometricShadowing::AshikhminPremoze: {
        geometricShadowing = NdotL * NdotV / (NdotL + NdotV - NdotL * NdotV);
        break; }

    case Options::BRDF::GeometricShadowing::Duer: {
        float3 LplusV = (lightDirection + viewDirection);
        geometricShadowing = dot(LplusV, LplusV) * pow(dot(LplusV, surfaceNormal), 4.0);
        break; }

    case Options::BRDF::GeometricShadowing::Neumann: {
        geometricShadowing = (NdotL * NdotV) / max(NdotL, NdotV);
        break; }

    case Options::BRDF::GeometricShadowing::Kelemen: {
        //geometricShadowing = (NdotL * NdotV) / square(LdotH);
        geometricShadowing = (NdotL * NdotV) / square(VdotH);
        break; }

    case Options::BRDF::GeometricShadowing::ModifiedKelemen: {
        static const float SqureRoot2OverPi = 0.797884560802865; // SqureRoot2OverPi = sqrt(2 / Pi)
        float k = materialAlpha * SqureRoot2OverPi;
        float gH = NdotV * k + (1.0 - k);
        geometricShadowing = (square(gH) * NdotL);
        break; }

    case Options::BRDF::GeometricShadowing::CookTorrence: {
        float NdotVProduct = 2.0 * NdotH * NdotV / VdotH;
        float NdotLProduct = 2.0 * NdotH * NdotL / VdotH;
        geometricShadowing = min(1.0, min(NdotVProduct, NdotLProduct));
        break; }

    case Options::BRDF::GeometricShadowing::Ward: {
        geometricShadowing = pow(NdotL * NdotV, 0.5);
        break; }

    case Options::BRDF::GeometricShadowing::Kurt: {
        //geometricShadowing = NdotL * NdotV / (VdotH * pow(NdotL * NdotV, materialRoughness));
        geometricShadowing = (VdotH * pow(NdotL * NdotV, materialRoughness)) / NdotL * NdotV;
        break; }

    case Options::BRDF::GeometricShadowing::WalterEtAl: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float2 calculation = 2.0 / (1.0 + sqrt(1.0 + materialAlpha * (1.0 - angleSquared) / angleSquared));
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::Beckman: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float2 calculation = angle / (materialAlpha * sqrt(1 - angleSquared));
        calculation = calculation < 1.6 ? (((3.535 * calculation) + (2.181 * square(calculation))) / (1.0 + (2.276 * calculation) + (2.577 * square(calculation)))) : 1.0;
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::GGX: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float2 calculation = (2.0 * angle) / (angle + sqrt(materialAlpha + (1.0 - materialAlpha) * angleSquared));
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::Schlick: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float2 calculation = angle / (angle * (1.0 - materialAlpha) + materialAlpha);
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::SchlickBeckman: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float k = materialAlpha * 0.797884560802865;
        float2 calculation = angle / (angle * (1.0 - k) + k);
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::SchlickGGX: {
        float2 angle = float2(NdotL, NdotV);
        float2 angleSquared = square(angle);
        float k = materialRoughness / 2.0;
        float2 calculation = angle / (angle * (1.0 - k) + k);
        geometricShadowing = calculation.x * calculation.y;
        break; }

    case Options::BRDF::GeometricShadowing::Implicit: {
        geometricShadowing = (NdotL * NdotV);
        break; }

    default:
        geometricShadowing = 0.0;
        break;
    };

    float3 fresnel;
    switch (Options::BRDF::Fresnel::Selection)
    {
    case Options::BRDF::Fresnel::Schlick: {
        fresnel = F0 + (1.0 - F0) * pow(1.0 - LdotH, 5.0);
        break; }

    case Options::BRDF::Fresnel::SphericalGaussian: {
        float power = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        fresnel = F0 + (1.0 - F0) * pow(2.0, power);
        break; }

    default:
        fresnel = 0.0;
        break;
    };

    float3 kd = lerp(float3(1, 1, 1) - fresnel, float3(0, 0, 0), materialMetallic);
    float3 diffuseRadiance = kd * materialAlbedo;

    // Lambert diffuse BRDF.
    // We don't scale by 1/PI for lighting & material units to be more convenient.
    // See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/

    float3 specularRadiance = (normalDistribution * fresnel * geometricShadowing) / max(Math::Epsilon, 4.0 * (NdotL * NdotV));
    switch (Options::BRDF::Debug::Selection)
    {
    case Options::BRDF::Debug::ShowAttenuation:
        return attenuation;

    case Options::BRDF::Debug::ShowDiffuse:
        return diffuseRadiance * attenuation;

    case Options::BRDF::Debug::ShowNormalDistribution:
        return normalDistribution * attenuation;

    case Options::BRDF::Debug::ShowGeometricShadow:
        return geometricShadowing * attenuation;

    case Options::BRDF::Debug::ShowFresnel:
        return fresnel * attenuation;

    case Options::BRDF::Debug::ShowSpecular:
        return specularRadiance * attenuation;
    };

    /*
        Maintain energy conservation:
        Energy conservation is a restriction on the reflection model that requires that the total amount of reflected light cannot be more than the incoming light.
        http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
    */
    float lambert;
    if (Options::BRDF::UseHalfLambert)
    {
        // http://developer.valvesoftware.com/wiki/Half_Lambert
        float halfLdotN = saturate((dot(surfaceNormal, lightDirection) * 0.5) + 0.5);
        lambert = square(halfLdotN);
    }
    else
    {
        lambert = NdotL;
    }

    return max(0.0, (diffuseRadiance + specularRadiance) * lightRadiance * attenuation * lambert);
}

float3 getSurfaceIrradiance(const float2 screenCoord, const float3 surfacePosition, const float3 surfaceNormal, const float3 materialAlbedo, const float materialRoughness, const float materialMetallic)
{
    switch (Options::BRDF::Debug::Selection)
    {
    case Options::BRDF::Debug::ShowAlbedo:
        return materialAlbedo;

    case Options::BRDF::Debug::ShowNormal:
        return surfaceNormal;

    case Options::BRDF::Debug::ShowRoughness:
        return materialRoughness;

    case Options::BRDF::Debug::ShowMetallic:
        return materialMetallic;
    };

    float materialAlpha = square(materialRoughness);
    float materialNonMetallic = 1.0 - materialMetallic;
    float3 viewDirection = -normalize(surfacePosition);
    float NdotV = saturate(dot(surfaceNormal, viewDirection));

    float3 diffuseContribution = materialAlbedo * materialNonMetallic;

    float3 surfaceIrradiance = 0.0;
    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		const Lights::DirectionalData lightData = Lights::directionalList[directionalIndex];

        surfaceIrradiance += getLightIrradiance(
            materialAlbedo, diffuseContribution,
            materialRoughness, materialAlpha,
            materialMetallic, materialNonMetallic,
            viewDirection, surfaceNormal, NdotV, 
            lightData.direction, lightData.radiance, 1.0);
    }

    const uint clusterOffset = getClusterOffset(screenCoord, surfacePosition.z);
    const uint2 clusterData = Lights::clusterDataList[clusterOffset];
    uint indexOffset = clusterData.x;
    const uint pointLightEnd = ((clusterData.y & 0x0000FFFF) + indexOffset);
    const uint spotLightEnd = (((clusterData.y >> 16) & 0x0000FFFF) + pointLightEnd);

    uint2 gridLocation = floor(screenCoord * Lights::ReciprocalTileSize.xy);
    float depth = ((surfacePosition.z - Camera::NearClip) * Camera::ReciprocalClipDistance);
    uint gridSlice = floor(depth * Lights::gridSize.z);

    while (indexOffset < pointLightEnd) 
    {
        const uint lightIndex = Lights::clusterIndexList[indexOffset++];
        const Lights::PointData lightData = Lights::pointList[lightIndex];

        float3 lightRay = (lightData.position - surfacePosition);
        float lightDistance = length(lightRay);
        float3 lightDirection = (lightRay / lightDistance);

        float attenuation = getFalloff(lightDistance, lightData.range);

        surfaceIrradiance += getLightIrradiance(
            materialAlbedo, diffuseContribution,
            materialRoughness, materialAlpha,
            materialMetallic, materialNonMetallic,
            viewDirection, surfaceNormal, NdotV,
            lightDirection, lightData.radiance, attenuation);
    };

    while (indexOffset < spotLightEnd)
    {
        const uint lightIndex = Lights::clusterIndexList[indexOffset++];
        const Lights::SpotData lightData = Lights::spotList[lightIndex];

        float3 lightRay = (lightData.position - surfacePosition);
        float lightDistance = length(lightRay);
        float3 lightDirection = (lightRay / lightDistance);

        float attenuation = getFalloff(lightDistance, lightData.range);
        attenuation *= getSpotFactor(lightData.direction, lightDirection, lightData.innerAngle, lightData.outerAngle, lightData.coneFalloff);

        surfaceIrradiance += getLightIrradiance(
            materialAlbedo, diffuseContribution,
            materialRoughness, materialAlpha,
            materialMetallic, materialNonMetallic,
            viewDirection, surfaceNormal, NdotV,
            lightDirection, lightData.radiance, attenuation);
    };

    return surfaceIrradiance;
}

// http://www.jordanstevenstechart.com/physically-based-rendering

struct LightData
{
    float3 surfaceNormal;
    float3 viewDirection;
    float3 reflectedViewDirection;
    float3 materialAlbedo;
    float materialRoughness;
    float materialAlpha;
    float materialMetallic;
    float3 lightDirection;
    float3 lightRadiance;
    float attenuation;
    float3 reflectedRadiance;
    float3 lightReflectDirection;
    float NdotV;
    float NdotL;
    float NdotH;
    float VdotH;
    float LdotH;
    float LdotV;
    float RdotV;

    float getNormalDistributionBeckmann(void)
    {
        float materialAlphaSquared = square(materialAlpha);
        float NdotHSquared = square(NdotH);
        float NdotHCubed = square(NdotHSquared);
        return (1.0 / (Math::Pi * materialAlphaSquared * NdotHCubed)) *
            exp((NdotHSquared - 1.0) / (materialAlphaSquared * NdotHSquared));
    }

    float getNormalDistributionGaussian(void)
    {
        float materialAlphaSquared = square(materialAlpha);
        float thetaH = acos(NdotH);
        return exp(-thetaH * thetaH / materialAlphaSquared);
    }

    float getNormalDistributionGGX(void)
    {
        float materialAlphaSquared = square(materialAlpha);
        float NdotHSquared = square(NdotH);
        float TangentNdotHSquared = (1.0 - NdotHSquared) / NdotHSquared;
        return Math::ReciprocalPi * square(materialRoughness / (NdotHSquared * (materialAlphaSquared + TangentNdotHSquared)));
    }

    float getNormalDistributionTrowbridgeReitz(void)
    {
        float materialAlphaSquared = square(materialAlpha);
        float NdotHSquared = square(NdotH);
        return materialAlphaSquared / (Math::Pi * square(NdotHSquared * (materialAlphaSquared - 1.0) + 1.0));
    }

    float getNormalDistribution(void)
    {
        switch (Options::BRDF::NormalDistribution::Selection)
        {
        case Options::BRDF::NormalDistribution::Beckmann:
            return getNormalDistributionBeckmann();

        case Options::BRDF::NormalDistribution::Gaussian:
            return getNormalDistributionGaussian();

        case Options::BRDF::NormalDistribution::GGX:
            return getNormalDistributionGGX();

        case Options::BRDF::NormalDistribution::TrowbridgeReitz:
            return getNormalDistributionTrowbridgeReitz();
        };

        return 1.0;
    }

    float getGeometricShadowingImplicit(void)
    {
        return (NdotL * NdotV);
    }

    float getGeometricShadowingAshikhminShirley(void)
    {
        return NdotL * NdotV / (LdotH * max(NdotL, NdotV));
    }

    float getGeometricShadowingAshikhminPremoze(void)
    {
        return NdotL * NdotV / (NdotL + NdotV - NdotL * NdotV);
    }

    float getGeometricShadowingDuer(void)
    {
        float3 LplusV = lightDirection + viewDirection;
        return dot(LplusV, LplusV) * pow(dot(LplusV, surfaceNormal), -4.0);
    }

    float getGeometricShadowingNeumann(void)
    {
        return (NdotL * NdotV) / max(NdotL, NdotV);
    }

    float getGeometricShadowingKelemen(void)
    {
        return (NdotL * NdotV) / square(VdotH);
    }

    float getGeometricShadowingModifiedKelemen(void)
    {
        static const float SqureRoot2OverPi = 0.797884560802865; // SqureRoot2OverPi = sqrt(2 / Pi)
        float modifiedmaterialAlphaSquared = materialRoughness * materialRoughness * SqureRoot2OverPi;
        float gH = NdotV * modifiedmaterialAlphaSquared + (1.0 - modifiedmaterialAlphaSquared);
        return (square(gH) * NdotL);
    }

    float getGeometricShadowingCookTorrence(void)
    {
        float NdotVProduct = (2.0 * (NdotH * NdotV)) / VdotH;
        float NdotLProduct = (2.0 * (NdotH * NdotL)) / VdotH;
        return min(1.0, min(NdotVProduct, NdotLProduct));
    }

    float getGeometricShadowingWard(void)
    {
        return pow(NdotL * NdotV, 0.5);
    }

    float getGeometricShadowingKurt(void)
    {
        return (VdotH * pow(NdotL * NdotV, materialRoughness)) / NdotL * NdotV;
    }

    float getGeometricShadowingWalterEtAl(float angle)
    {
        float materialAlphaSquared = square(materialAlpha);
        float angleSquared = square(angle);
        return 2.0 / (1.0 + sqrt(1.0 + materialAlphaSquared * (1.0 - angleSquared) / angleSquared));
    }

    float getGeometricShadowingBeckman(float angle)
    {
        float angleSquared = square(angle);
        float c = angle / (materialAlpha * sqrt(1.0 - angleSquared));
        float cSquared = square(c);
        return c < 1.6 ? (((3.535 * c) + (2.181 * cSquared)) / (1.0 + (2.276 * c) + (2.577 * cSquared))) : 1.0;
    }

    float getGeometricShadowingGGX(float angle)
    {
        float materialAlphaSquared = square(materialAlpha);
        return (2.0 * angle) / (angle + sqrt(materialAlphaSquared + ((1.0 - materialAlphaSquared) * square(angle))));
    }

    float getGeometricShadowingSchlick(float angle)
    {
        float materialAlphaSquared = square(materialAlpha);
        return angle / ((angle * (1.0 - materialAlphaSquared)) + materialAlphaSquared);
    }

    float getGeometricShadowingSchlickBeckman(float angle)
    {
        static const float SquareRootTwoOverPi = sqrt(2.0 / Math::Pi);
        float materialAlphaSquared = square(materialAlpha);
        float k = materialAlphaSquared * SquareRootTwoOverPi;
        return angle / ((angle * (1.0 - k)) + k);
    }

    float getGeometricShadowingSchlickGGX(float angle)
    {
        float k = materialAlpha / 2.0;
        return angle / ((angle * (1.0 - k)) + k);
    }

    float getGeometricShadowing(void)
    {
        switch (Options::BRDF::GeometricShadowing::Selection)
        {
        case Options::BRDF::GeometricShadowing::AshikhminShirley:
            return getGeometricShadowingAshikhminShirley();

        case Options::BRDF::GeometricShadowing::AshikhminPremoze:
            return getGeometricShadowingAshikhminPremoze();

        case Options::BRDF::GeometricShadowing::Duer:
            return getGeometricShadowingDuer();

        case Options::BRDF::GeometricShadowing::Neumann:
            return getGeometricShadowingNeumann();

        case Options::BRDF::GeometricShadowing::Kelemen:
            return getGeometricShadowingKelemen();

        case Options::BRDF::GeometricShadowing::ModifiedKelemen:
            return getGeometricShadowingModifiedKelemen();

        case Options::BRDF::GeometricShadowing::CookTorrence:
            return getGeometricShadowingCookTorrence();

        case Options::BRDF::GeometricShadowing::Ward:
            return getGeometricShadowingWard();

        case Options::BRDF::GeometricShadowing::Kurt:
            return getGeometricShadowingKurt();

        case Options::BRDF::GeometricShadowing::WalterEtAl:
            return getGeometricShadowingWalterEtAl(NdotL) * getGeometricShadowingWalterEtAl(NdotV);

        case Options::BRDF::GeometricShadowing::Beckman:
            return getGeometricShadowingBeckman(NdotL) * getGeometricShadowingBeckman(NdotV);

        case Options::BRDF::GeometricShadowing::GGX:
            return getGeometricShadowingGGX(NdotL) * getGeometricShadowingGGX(NdotV);

        case Options::BRDF::GeometricShadowing::Schlick:
            return getGeometricShadowingSchlick(NdotL) * getGeometricShadowingSchlick(NdotV);

        case Options::BRDF::GeometricShadowing::SchlickBeckman:
            return getGeometricShadowingSchlickBeckman(NdotL) * getGeometricShadowingSchlickBeckman(NdotV);

        case Options::BRDF::GeometricShadowing::SchlickGGX:
            return getGeometricShadowingSchlickGGX(NdotL) * getGeometricShadowingSchlickGGX(NdotV);

        case Options::BRDF::GeometricShadowing::Implicit:
            return getGeometricShadowingImplicit();
        };

        return 1.0;
    }

    float3 getFresnelSchlick(void)
    {
        return reflectedRadiance + ((1.0 - reflectedRadiance) * pow((1.0 - VdotH), 5.0));
    }

    float3 getFresnelSphericalGaussian(void)
    {
        float gaussianPower = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        return reflectedRadiance + ((1.0 - reflectedRadiance) * pow(2.0, gaussianPower));
    }

    float3 getFresnel(void)
    {
        switch (Options::BRDF::Fresnel::Selection)
        {
        case Options::BRDF::Fresnel::Schlick:
            return reflectedRadiance * getFresnelSchlick();

        case Options::BRDF::Fresnel::SphericalGaussian:
            return reflectedRadiance  * getFresnelSphericalGaussian();
        };

        return reflectedRadiance;
    }

    float getLambert(void)
    {
        if (Options::BRDF::UseHalfLambert)
        {
            // http://developer.valvesoftware.com/wiki/Half_Lambert
            float halfLdotN = saturate((dot(surfaceNormal, lightDirection) * 0.5) + 0.5);
            return square(halfLdotN);
        }
        else
        {
            return NdotL;
        }
    }

    float3 getSpecularColor(void)
    {
        return ((getNormalDistribution() * getGeometricShadowing() * getFresnel()) / (4.0 * (NdotL * NdotV)));
    }

    float3 getIrradiance(float3 _lightDirection, float3 _lightRadiance, float _attenuation)
    {
        lightDirection = _lightDirection;
        lightRadiance = _lightRadiance;
        attenuation = _attenuation;
        reflectedRadiance = lerp(materialAlbedo, lightRadiance, materialMetallic);
        lightReflectDirection = reflect(-lightDirection, surfaceNormal);
        float3 halfDirection = normalize(viewDirection + lightDirection);
        NdotL = saturate(dot(surfaceNormal, lightDirection));
        NdotH = saturate(dot(surfaceNormal, halfDirection));
        VdotH = saturate(dot(viewDirection, halfDirection));
        LdotH = saturate(dot(lightDirection, halfDirection));
        LdotV = saturate(dot(lightDirection, viewDirection));
        RdotV = saturate(dot(lightReflectDirection, viewDirection));

        switch (Options::BRDF::Debug::Selection)
        {
        case Options::BRDF::Debug::ShowAttenuation:
            return attenuation;

        case Options::BRDF::Debug::ShowDistribution:
            return getNormalDistribution();

        case Options::BRDF::Debug::ShowFresnel:
            return getFresnel();

        case Options::BRDF::Debug::ShowGeometricShadow:
            return getGeometricShadowing();
        };

        return (getSpecularColor() * getLambert() * attenuation * lightRadiance);
    }
};

float getFalloff(float distance, float range)
{
    float denominator = (pow(distance, 2.0) + 1.0);
    float attenuation = pow((distance / range), 4.0);
    return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
}

uint getClusterOffset(float2 screenPosition, float surfaceDepth)
{
    uint2 gridLocation = floor(screenPosition * Lights::ReciprocalTileSize.xy);

    float depth = ((surfaceDepth - Camera::NearClip) * Camera::ReciprocalClipDistance);
    uint gridSlice = floor(depth * Lights::gridSize.z);

    return ((((gridSlice * Lights::gridSize.y) + gridLocation.y) * Lights::gridSize.x) + gridLocation.x);
}

float3 getSurfaceIrradiance(float2 screenCoord, float3 surfacePosition, float3 surfaceNormal, float3 materialAlbedo, float materialRoughness, float materialMetallic)
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

    float3 viewDirection = -normalize(surfacePosition);

    LightData data;
    data.surfaceNormal = surfaceNormal;
    data.materialAlbedo = materialAlbedo;
    data.materialRoughness = materialRoughness;
    data.materialAlpha = pow(materialRoughness, 2.0);;
    data.materialMetallic = materialMetallic;
    data.viewDirection = viewDirection;
    data.reflectedViewDirection = reflect(-viewDirection, surfaceNormal);
    data.NdotV = saturate(dot(surfaceNormal, viewDirection));

    float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		const Lights::DirectionalData lightData = Lights::directionalList[directionalIndex];

        surfaceIrradiance += data.getIrradiance(lightData.direction, lightData.radiance, 1.0);
    }

    const uint clusterOffset = getClusterOffset(screenCoord, surfacePosition.z);
    const uint2 clusterData = Lights::clusterDataList[clusterOffset];
    uint indexOffset = clusterData.x;
    const uint pointLightEnd = ((clusterData.y & 0x0000FFFF) + indexOffset);
    const uint spotLightEnd = (((clusterData.y >> 16) & 0x0000FFFF) + pointLightEnd);

    while (indexOffset < pointLightEnd)
    {
        const uint lightIndex = Lights::clusterIndexList[indexOffset++];
        const Lights::PointData lightData = Lights::pointList[lightIndex];

        float3 lightRay = (lightData.position - surfacePosition);
        float3 centerToRay = (lightRay - (dot(lightRay, data.reflectedViewDirection) * data.reflectedViewDirection));
        float3 closestPoint = (lightRay + (centerToRay * max(0.0, (lightData.radius / length(centerToRay)))));
        float lightDistance = length(closestPoint);
        float3 lightDirection = normalize(closestPoint);
        float attenuation = getFalloff(lightDistance, lightData.range);
        surfaceIrradiance += data.getIrradiance(lightDirection, lightData.radiance, attenuation);
    };

    while (indexOffset < spotLightEnd)
    {
        const uint lightIndex = Lights::clusterIndexList[indexOffset++];
        const Lights::SpotData lightData = Lights::spotList[lightIndex];

        float3 lightRay = (lightData.position - surfacePosition);
        float lightDistance = length(lightRay);
        float3 lightDirection = (lightRay / lightDistance);
        float rho = max(0.0, (dot(lightData.direction, -lightDirection)));
        float spotFactor = pow(max(0.0, (rho - lightData.outerAngle) / (lightData.innerAngle - lightData.outerAngle)), lightData.coneFalloff);
        float attenuation = (getFalloff(lightDistance, lightData.range) * spotFactor);
        surfaceIrradiance += data.getIrradiance(lightDirection, lightData.radiance, attenuation);
    };

    return surfaceIrradiance;
}

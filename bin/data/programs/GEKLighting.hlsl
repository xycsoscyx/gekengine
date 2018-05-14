// http://www.jordanstevenstechart.com/physically-based-rendering

struct LightData
{
    float3 surfaceNormal;
    float3 viewDirection;
    float3 reflectedViewDirection;
    float3 materialAlbedo;
    float materialRoughness;
    float materialMetallic;
    float3 lightDirection;
    float3 lightRadiance;
    float attenuation;
    float3 attenuatedColor;
    float3 reflectedRadiance;
    float3 lightReflectDirection;
    float NdotV;
    float NdotL;
    float NdotH;
    float VdotH;
    float LdotH;
    float LdotV;
    float RdotV;

    void prepare(void)
    {
        attenuatedColor = attenuation * lightRadiance;
        reflectedRadiance = lerp(materialAlbedo, lightRadiance, materialMetallic);
        lightReflectDirection = reflect(-lightDirection, surfaceNormal);
        float3 halfDirection = normalize(viewDirection + lightDirection);
        NdotL = saturate(dot(surfaceNormal, lightDirection));
        NdotH = saturate(dot(surfaceNormal, halfDirection));
        VdotH = saturate(dot(viewDirection, halfDirection));
        LdotH = saturate(dot(lightDirection, halfDirection));
        LdotV = saturate(dot(lightDirection, viewDirection));
        RdotV = saturate(dot(lightReflectDirection, viewDirection));
    }

    // schlick functions
    float getSchlickFresnel(float index)
    {
        float x = clamp(1.0 - index, 0.0, 1.0);
        float x2 = square(x);
        return x2 * x2 * x;
    }

    float3 getFresnelLerp(float3 x, float3 y, float delta)
    {
        float transition = getSchlickFresnel(delta);
        return lerp(x, y, transition);
    }

    float getNormalDistributionBeckmann(void)
    {
        float roughnessSquared = square(materialRoughness);
        float NdotHSquared = square(NdotH);
        return (1.0 / (Math::Pi * roughnessSquared * square(NdotHSquared))) * exp((NdotHSquared - 1.0) / (roughnessSquared * NdotHSquared));
    }

    float getNormalDistributionGaussian(void)
    {
        float roughnessSquared = square(materialRoughness);
        float thetaH = acos(NdotH);
        return exp(-thetaH * thetaH / roughnessSquared);
    }

    float getNormalDistributionGGX(void)
    {
        float roughnessSquared = square(materialRoughness);
        float NdotHSquared = square(NdotH);
        float TangentNdotHSquared = (1.0 - NdotHSquared) / NdotHSquared;
        return Math::ReciprocalPi * square(materialRoughness / (NdotHSquared * (roughnessSquared + TangentNdotHSquared)));
    }

    float getNormalDistributionTrowbridgeReitz(void)
    {
        float roughnessSquared = square(materialRoughness);
        float distribution = square(NdotH) * (roughnessSquared - 1.0) + 1.0;
        return roughnessSquared / (Math::Pi * square(distribution));
    }

    float3 getNormalDistribution(void)
    {
        //Specular calculations
        float3 normalDistribution = reflectedRadiance;
        switch (Options::BRDF::NormalDistribution::Selection)
        {
        case Options::BRDF::NormalDistribution::Beckmann:
            normalDistribution *= getNormalDistributionBeckmann();
            break;

        case Options::BRDF::NormalDistribution::Gaussian:
            normalDistribution *= getNormalDistributionGaussian();
            break;

        case Options::BRDF::NormalDistribution::GGX:
            normalDistribution *= getNormalDistributionGGX();
            break;

        case Options::BRDF::NormalDistribution::TrowbridgeReitz:
            normalDistribution *= getNormalDistributionTrowbridgeReitz();
            break;
        };

        return normalDistribution;
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
        //	return (NdotL * NdotV)/ (LdotH * LdotH);           //this
        return (NdotL * NdotV) / (VdotH * VdotH);       //or this?
    }

    float getGeometricShadowingModifiedKelemen(void)
    {
        static const float SqureRoot2OverPi = 0.797884560802865; // SqureRoot2OverPi = sqrt(2 / Pi)
        float modifiedRoughnessSquared = materialRoughness * materialRoughness * SqureRoot2OverPi;
        float gH = NdotV * modifiedRoughnessSquared + (1.0 - modifiedRoughnessSquared);
        return (square(gH) * NdotL);
    }

    float getGeometricShadowingCookTorrence(void)
    {
        return min(1.0, min(2.0 * NdotH * NdotV / VdotH, 2 * NdotH * NdotL / VdotH));
    }

    float getGeometricShadowingWard(void)
    {
        return pow(NdotL * NdotV, 0.5);
    }

    float getGeometricShadowingKurt(void)
    {
        return (VdotH * pow(NdotL * NdotV, materialRoughness)) / NdotL * NdotV;
    }

    //SmithModelsBelow
    //Gs = F(NdotL) * F(NdotV);
    float getGeometricShadowingWalterEtAl(void)
    {
        float roughnessSquared = square(materialRoughness);
        float NdotLSquared = square(NdotL);
        float NdotVSquared = square(NdotV);
        float SmithNdotL = 2.0 / (1.0 + sqrt(1.0 + roughnessSquared * (1.0 - NdotLSquared) / (NdotLSquared)));
        float SmithNdotV = 2.0 / (1.0 + sqrt(1.0 + roughnessSquared * (1.0 - NdotVSquared) / (NdotVSquared)));
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowingBeckman(void)
    {
        float roughnessSquared = square(materialRoughness);
        float NdotLSquared = square(NdotL);
        float NdotVSquared = square(NdotV);
        float calulationL = (NdotL) / (roughnessSquared * sqrt(1 - NdotLSquared));
        float calulationV = (NdotV) / (roughnessSquared * sqrt(1 - NdotVSquared));
        float SmithNdotL = calulationL < 1.6 ? (((3.535 * calulationL) + (2.181 * calulationL * calulationL)) / (1.0 + (2.276 * calulationL) + (2.577 * calulationL * calulationL))) : 1.0;
        float SmithNdotV = calulationV < 1.6 ? (((3.535 * calulationV) + (2.181 * calulationV * calulationV)) / (1.0 + (2.276 * calulationV) + (2.577 * calulationV * calulationV))) : 1.0;
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowingGGX(void)
    {
        float roughnessSquared = square(materialRoughness);
        float NdotLSquared = square(NdotL);
        float NdotVSquared = square(NdotV);
        float SmithNdotL = (2.0 * NdotL) / (NdotL + sqrt(roughnessSquared + (1.0 - roughnessSquared) * NdotLSquared));
        float SmithNdotV = (2.0 * NdotV) / (NdotV + sqrt(roughnessSquared + (1.0 - roughnessSquared) * NdotVSquared));
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowingSchlick(void)
    {
        float roughnessSquared = square(materialRoughness);
        float SmithNdotL = (NdotL) / (NdotL * (1.0 - roughnessSquared) + roughnessSquared);
        float SmithNdotV = (NdotV) / (NdotV * (1.0 - roughnessSquared) + roughnessSquared);
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowingSchlickBeckman(void)
    {
        float roughnessSquared = square(materialRoughness);
        float k = roughnessSquared * 0.797884560802865;
        float SmithNdotL = (NdotL) / (NdotL * (1.0 - k) + k);
        float SmithNdotV = (NdotV) / (NdotV * (1.0 - k) + k);
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowingSchlickGGX(void)
    {
        float k = materialRoughness / 2.0;
        float SmithNdotL = (NdotL) / (NdotL * (1.0 - k) + k);
        float SmithNdotV = (NdotV) / (NdotV * (1.0 - k) + k);
        return (SmithNdotL * SmithNdotV);
    }

    float getGeometricShadowing(void)
    {
        float geometricShadow;
        switch (Options::BRDF::GeometricShadowing::Selection)
        {
        case Options::BRDF::GeometricShadowing::AshikhminShirley:
            geometricShadow = getGeometricShadowingAshikhminShirley();
            break;

        case Options::BRDF::GeometricShadowing::AshikhminPremoze:
            geometricShadow = getGeometricShadowingAshikhminPremoze();
            break;

        case Options::BRDF::GeometricShadowing::Duer:
            geometricShadow = getGeometricShadowingDuer();
            break;

        case Options::BRDF::GeometricShadowing::Neumann:
            geometricShadow = getGeometricShadowingNeumann();
            break;

        case Options::BRDF::GeometricShadowing::Kelemen:
            geometricShadow = getGeometricShadowingKelemen();
            break;

        case Options::BRDF::GeometricShadowing::ModifiedKelemen:
            geometricShadow = getGeometricShadowingModifiedKelemen();
            break;

        case Options::BRDF::GeometricShadowing::CookTorrence:
            geometricShadow = getGeometricShadowingCookTorrence();
            break;

        case Options::BRDF::GeometricShadowing::Ward:
            geometricShadow = getGeometricShadowingWard();
            break;

        case Options::BRDF::GeometricShadowing::Kurt:
            geometricShadow = getGeometricShadowingKurt();
            break;

        case Options::BRDF::GeometricShadowing::WalterEtAl:
            geometricShadow = getGeometricShadowingWalterEtAl();
            break;

        case Options::BRDF::GeometricShadowing::Beckman:
            geometricShadow = getGeometricShadowingBeckman();
            break;

        case Options::BRDF::GeometricShadowing::GGX:
            geometricShadow = getGeometricShadowingGGX();
            break;

        case Options::BRDF::GeometricShadowing::Schlick:
            geometricShadow = getGeometricShadowingSchlick();
            break;

        case Options::BRDF::GeometricShadowing::SchlickBeckman:
            geometricShadow = getGeometricShadowingSchlickBeckman();
            break;

        case Options::BRDF::GeometricShadowing::SchlickGGX:
            geometricShadow = getGeometricShadowingSchlickGGX();
            break;

        case Options::BRDF::GeometricShadowing::Implicit:
            geometricShadow = getGeometricShadowingImplicit();
            break;

        default:
            geometricShadow = 1.0;
            break;
        };

        return geometricShadow;
    }

    float3 getFresnelSchlick(void)
    {
        return reflectedRadiance + (1.0 - reflectedRadiance)* getSchlickFresnel(LdotH);
    }

    float3 getFresnelSphericalGaussian(void)
    {
        float gaussianPower = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        return reflectedRadiance + (1.0 - reflectedRadiance) * pow(2.0, gaussianPower);
    }

    float3 getFresnel(void)
    {
        float3 fresnel = reflectedRadiance;
        switch (Options::BRDF::Fresnel::Selection)
        {
        case Options::BRDF::Fresnel::Schlick:
            fresnel *= getFresnelSchlick();
            break;

        case Options::BRDF::Fresnel::SphericalGaussian:
            fresnel *= getFresnelSphericalGaussian();
            break;
        };

        return fresnel;
    }

    float getLambert(void)
    {
        if (Options::BRDF::UseHalfLambert)
        {
            // http://developer.valvesoftware.com/wiki/Half_Lambert
            float halfLdotN = ((dot(surfaceNormal, lightDirection) * 0.5) + 0.5);
            return pow(halfLdotN, 2.0);
        }
        else
        {
            return NdotL;
        }
    }

    float3 getDiffuseColor(void)
    {
        float fresnelLight = getSchlickFresnel(NdotL);
        float fresnelView = getSchlickFresnel(NdotV);
        float fresnelDiffuse90 = 0.5 + 2.0 * LdotH * LdotH * materialRoughness;
        float f0 = lerp(1.0, fresnelDiffuse90, fresnelLight) * lerp(1.0, fresnelDiffuse90, fresnelView);
        return f0 * lerp(materialAlbedo, 1.0, materialMetallic);
    }

    float3 getSpecularColor(void)
    {
        return ((getNormalDistribution() * getFresnel() * getGeometricShadowing()) / (4.0 * (NdotL * NdotV)));
    }

    float3 getIrradiance(void)
    {
        prepare();
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

        return max(0.0, ((getDiffuseColor() + getSpecularColor()) * getLambert() * attenuation * lightRadiance));
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
    float materialAlpha;
    if (Options::BRDF::UseDisneyAlpha)
    {
        // materialAlpha modifications by Disney - s2012_pbs_disney_brdf_notes_v2.pdf
        materialAlpha = pow(materialRoughness, 2.0);
    }
    else
    {
        // reduce roughness range from [0 .. 1] to [0.5 .. 1]
        materialAlpha = pow(0.5 + materialRoughness * 0.5, 2.0);
    }

    switch (Options::BRDF::Debug::Selection)
    {
    case Options::BRDF::Debug::ShowAlbedo:
        return materialAlbedo;

    case Options::BRDF::Debug::ShowNormal:
        return surfaceNormal;

    case Options::BRDF::Debug::ShowRoughness:
        return materialAlpha;

    case Options::BRDF::Debug::ShowMetallic:
        return materialMetallic;
    };

    float3 viewDirection = -normalize(surfacePosition);

    LightData data;
    data.surfaceNormal = surfaceNormal;
    data.materialAlbedo = materialAlbedo;
    data.materialRoughness = materialAlpha;
    data.materialMetallic = materialMetallic;
    data.viewDirection = viewDirection;
    data.reflectedViewDirection = reflect(-viewDirection, surfaceNormal);
    data.NdotV = saturate(dot(surfaceNormal, viewDirection));

    float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		const Lights::DirectionalData lightData = Lights::directionalList[directionalIndex];

        data.lightDirection = lightData.direction;
        data.lightRadiance = lightData.radiance;
        data.attenuation = 1.0;
        surfaceIrradiance += data.getIrradiance();
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

        data.lightDirection = lightDirection;
        data.lightRadiance = lightData.radiance;
        data.attenuation = attenuation;
        surfaceIrradiance += data.getIrradiance();
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

        data.lightDirection = lightDirection;
        data.lightRadiance = lightData.radiance;
        data.attenuation = attenuation;
        surfaceIrradiance += data.getIrradiance();
    };

    return surfaceIrradiance;
}

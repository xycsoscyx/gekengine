// http://www.jordanstevenstechart.com/physically-based-rendering

struct LightData
{
    float3 surfaceNormal;
    float3 viewDirection;
    float3 reflectedViewDirection;
    float NdotV;
    float3 materialAlbedo;
    float materialRoughness;
    float materialMetallic;
    float3 lightDirection;
    float3 lightRadiance;
    float attenuation;
    float3 attenuatedColor;
    float3 reflectedRadiance;
    float3 lightReflectDirection;
    float NdotL;
    float3 halfDirection;
    float NdotH;
    float VdotH;
    float LdotH;
    float LdotV;
    float RdotV;

    void prepare(void)
    {
        float3 lightReflectDirection = reflect(-lightDirection, surfaceNormal);
        NdotL = max(0.0, dot(surfaceNormal, lightDirection));
        halfDirection = normalize(viewDirection + lightDirection);
        NdotH = max(0.0, dot(surfaceNormal, halfDirection));
        VdotH = max(0.0, dot(viewDirection, halfDirection));
        LdotH = max(0.0, dot(lightDirection, halfDirection));
        LdotV = max(0.0, dot(lightDirection, viewDirection));
        RdotV = max(0.0, dot(lightReflectDirection, viewDirection));
        attenuatedColor = attenuation * lightRadiance;
        reflectedRadiance = lerp(lightRadiance, materialAlbedo, materialMetallic);
    }

    // schlick functions
    float getSchlickFresnel(float i)
    {
        float x = clamp(1.0 - i, 0.0, 1.0);
        float x2 = x * x;
        return x2 * x2 * x;
    }

    float3 getFresnelLerp(float3 x, float3 y, float d)
    {
        float t = getSchlickFresnel(d);
        return lerp(x, y, t);
    }

    float getNormalDistributionBeckmann(void)
    {
        float roughnessSqr = square(materialRoughness);
        float NdotHSqr = square(NdotH);
        return max(0.000001, (1.0 / (3.1415926535 * roughnessSqr * square(NdotHSqr))) * exp((NdotHSqr - 1.0) / (roughnessSqr * NdotHSqr)));
    }

    float getNormalDistributionGaussian(void)
    {
        float roughnessSqr = square(materialRoughness);
        float thetaH = acos(NdotH);
        return exp(-thetaH * thetaH / roughnessSqr);
    }

    float getNormalDistributionGGX(void)
    {
        float roughnessSqr = square(materialRoughness);
        float NdotHSqr = square(NdotH);
        float TanNdotHSqr = (1 - NdotHSqr) / NdotHSqr;
        return Math::ReciprocalPi * square(materialRoughness / (NdotHSqr * (roughnessSqr + TanNdotHSqr)));
    }

    float getNormalDistributionTrowbridgeReitz(void)
    {
        float roughnessSqr = square(materialRoughness);
        float Distribution = square(NdotH) * (roughnessSqr - 1.0) + 1.0;
        return roughnessSqr / (Math::Pi * square(Distribution));
    }

    float3 getNormalDistribution(void)
    {
        //Specular calculations
        float3 distribution = reflectedRadiance;
        switch (Options::BRDF::NormalDistribution::Selection)
        {
        case Options::BRDF::NormalDistribution::Beckmann:
            distribution *= getNormalDistributionBeckmann();
            break;

        case Options::BRDF::NormalDistribution::Gaussian:
            distribution *= getNormalDistributionGaussian();
            break;

        case Options::BRDF::NormalDistribution::GGX:
            distribution *= getNormalDistributionGGX();
            break;

        case Options::BRDF::NormalDistribution::TrowbridgeReitz:
            distribution *= getNormalDistributionTrowbridgeReitz();
            break;
        };

        return distribution;
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
        float3 LpV = lightDirection + viewDirection;
        float Gs = dot(LpV, LpV) * pow(dot(LpV, surfaceNormal), -4);
        return  (Gs);
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
        float c = 0.797884560802865; // c = sqrt(2 / Pi)
        float k = materialRoughness * materialRoughness * c;
        float gH = NdotV * k + (1.0 - k);
        return (gH * gH * NdotL);
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
        float alphaSqr = materialRoughness * materialRoughness;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float SmithL = 2.0 / (1.0 + sqrt(1.0 + alphaSqr * (1.0 - NdotLSqr) / (NdotLSqr)));
        float SmithV = 2.0 / (1.0 + sqrt(1.0 + alphaSqr * (1.0 - NdotVSqr) / (NdotVSqr)));
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float getGeometricShadowingBeckman(void)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float calulationL = (NdotL) / (roughnessSqr * sqrt(1 - NdotLSqr));
        float calulationV = (NdotV) / (roughnessSqr * sqrt(1 - NdotVSqr));
        float SmithL = calulationL < 1.6 ? (((3.535 * calulationL) + (2.181 * calulationL * calulationL)) / (1.0 + (2.276 * calulationL) + (2.577 * calulationL * calulationL))) : 1.0;
        float SmithV = calulationV < 1.6 ? (((3.535 * calulationV) + (2.181 * calulationV * calulationV)) / (1.0 + (2.276 * calulationV) + (2.577 * calulationV * calulationV))) : 1.0;
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float getGeometricShadowingGGX(void)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float SmithL = (2.0 * NdotL) / (NdotL + sqrt(roughnessSqr + (1.0 - roughnessSqr) * NdotLSqr));
        float SmithV = (2.0 * NdotV) / (NdotV + sqrt(roughnessSqr + (1.0 - roughnessSqr) * NdotVSqr));
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float getGeometricShadowingSchlick(void)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float SmithL = (NdotL) / (NdotL * (1.0 - roughnessSqr) + roughnessSqr);
        float SmithV = (NdotV) / (NdotV * (1.0 - roughnessSqr) + roughnessSqr);
        return (SmithL * SmithV);getGeometricShadowing
    }

    float getGeometricShadowingSchlickBeckman(void)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float k = roughnessSqr * 0.797884560802865;
        float SmithL = (NdotL) / (NdotL * (1.0 - k) + k);
        float SmithV = (NdotV) / (NdotV * (1.0 - k) + k);
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float getGeometricShadowingSchlickGGX(void)
    {
        float k = materialRoughness / 2.0;
        float SmithL = (NdotL) / (NdotL * (1.0 - k) + k);
        float SmithV = (NdotV) / (NdotV * (1.0 - k) + k);
        float Gs = (SmithL * SmithV);
        return Gs;
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
        float power = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        return reflectedRadiance + (1.0 - reflectedRadiance) * pow(2, power);
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
        float FresnelLight = getSchlickFresnel(NdotL);
        float FresnelView = getSchlickFresnel(NdotV);
        float FresnelDiffuse90 = 0.5 + 2.0 * LdotH * LdotH * materialRoughness;
        float f0 = lerp(1, FresnelDiffuse90, FresnelLight) * lerp(1, FresnelDiffuse90, FresnelView);
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

        return ((getDiffuseColor() + getSpecularColor()) * getLambert() * attenuation * lightRadiance);
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

    LightData data;
    data.surfaceNormal = surfaceNormal;
    data.materialAlbedo = materialAlbedo;
    data.materialRoughness = materialRoughness * 1.5 + 0.5;
    data.materialMetallic = materialMetallic;
    data.viewDirection = -normalize(surfacePosition);
    data.reflectedViewDirection = reflect(-data.viewDirection, data.surfaceNormal);
    data.NdotV = max(0.0, dot(data.surfaceNormal, data.viewDirection));

    float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
		const Lights::DirectionalData lightData = Lights::directionalList[directionalIndex];

        data.lightDirection = lightData.direction;
        data.lightRadiance = lightData.radiance;
        data.attenuation = 1.0;
        surfaceIrradiance += data.Irradiance();
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
        surfaceIrradiance += data.Irradiance();
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
        surfaceIrradiance += data.Irradiance();
    };

    return surfaceIrradiance;
}

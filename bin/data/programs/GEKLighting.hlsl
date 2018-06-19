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
    float materialNonMetallic;
    float3 lightDirection;
    float3 lightReflectDirection;
    float3 lightRadiance;
    float3 diffuseRadiance;
    float3 specularRadiance;
    float attenuation;
    float NdotV;
    float NdotL;
    float NdotH;
    float VdotH;
    float LdotH;
    float LdotV;
    float RdotV;

    void initialize(float2 _screenCoord, float3 _surfacePosition, float3 _surfaceNormal, float3 _materialAlbedo, float _materialRoughness, float _materialMetallic)
    {
        surfaceNormal = _surfaceNormal;
        materialAlbedo = _materialAlbedo;
        materialRoughness = _materialRoughness;
        materialAlpha = pow(_materialRoughness, 2.0);;
        materialMetallic = _materialMetallic;
        materialNonMetallic = 1.0 - _materialMetallic;
        viewDirection = -normalize(_surfacePosition);
        reflectedViewDirection = reflect(-viewDirection, _surfaceNormal);
        diffuseRadiance = _materialAlbedo * materialNonMetallic;
        NdotV = saturate(dot(_surfaceNormal, viewDirection));
    }

    float getNormalDistributionBeckmann(void)
    {
        float NdotHSquared = square(NdotH);
        float NdotHCubed = square(NdotHSquared);
        return saturate((1.0 / (Math::Pi * materialAlpha * NdotHCubed)) * exp((NdotHSquared - 1.0) / (materialAlpha * NdotHSquared)));
    }

    float getNormalDistributionGaussian(void)
    {
        float thetaH = acos(NdotH);
        return exp(-thetaH * thetaH / materialAlpha);
    }

    float getNormalDistributionGGX(void)
    {
        float NdotHSquared = square(NdotH);
        float TangentNdotHSquared = (1.0 - NdotHSquared) / NdotHSquared;
        return Math::ReciprocalPi * square(materialRoughness / (NdotHSquared * (materialAlpha + TangentNdotHSquared)));
    }

    float getNormalDistributionTrowbridgeReitz(void)
    {
        float NdotHSquared = square(NdotH);
        return materialAlpha / (Math::Pi * square(NdotHSquared * (materialAlpha - 1.0) + 1.0));
    }

    float getNormalDistribution(void)
    {
        float normalDistribution;
        switch (Options::BRDF::NormalDistribution::Selection)
        {
        case Options::BRDF::NormalDistribution::Beckmann:
            normalDistribution = getNormalDistributionBeckmann();
            break;

        case Options::BRDF::NormalDistribution::Gaussian:
            normalDistribution = getNormalDistributionGaussian();
            break;

        case Options::BRDF::NormalDistribution::GGX:
            normalDistribution = getNormalDistributionGGX();
            break;

        case Options::BRDF::NormalDistribution::TrowbridgeReitz:
            normalDistribution = getNormalDistributionTrowbridgeReitz();
            break;

        default:
            normalDistribution = 0.0;
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
        float3 LplusV = (lightDirection + viewDirection);
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
        float k = materialAlpha * SqureRoot2OverPi;
        float gH = NdotV * k + (1.0 - k);
        return (square(gH) * NdotL);
    }

    float getGeometricShadowingCookTorrence(void)
    {
        float NdotVProduct = 2.0 * NdotH * NdotV / VdotH;
        float NdotLProduct = 2.0 * NdotH * NdotL / VdotH;
        return min(1.0, min(NdotVProduct, NdotLProduct));
    }

    float getGeometricShadowingWard(void)
    {
        return pow(NdotL * NdotV, 0.5);
    }

    float getGeometricShadowingKurt(void)
    {
        return NdotL * NdotV / (VdotH * pow(NdotL * NdotV, materialRoughness));
    }

    float getGeometricShadowingWalterEtAl(float angle)
    {
        float angleSquared = square(angle);
        return 2.0 / (1.0 + sqrt(1.0 + materialRoughness * (1.0 - angleSquared) / angleSquared));
    }

    float getGeometricShadowingBeckman(float angle)
    {
        float angleSquared = square(angle);
        float calulation = (NdotL) / (materialAlpha * sqrt(1 - angleSquared));
        return calulation < 1.6 ? (((3.535 * calulation) + (2.181 * calulation * calulation)) / (1 + (2.276 * calulation) + (2.577 * calulation * calulation))) : 1.0;
    }

    float getGeometricShadowingGGX(float angle)
    {
        float angleSquared = square(angle);
        return (2.0 * angle) / (angle + sqrt(materialAlpha + (1.0 - materialAlpha) * angleSquared));
    }

    float getGeometricShadowingSchlick(float angle)
    {
        return angle / (angle * (1.0 - materialAlpha) + materialAlpha);
    }

    float getGeometricShadowingSchlickBeckman(float angle)
    {
        float k = materialAlpha * 0.797884560802865;
        return angle / (angle * (1.0 - k) + k);
    }

    float getGeometricShadowingSchlickGGX(float angle)
    {
        float k = materialRoughness / 2.0;
        return angle / (angle * (1.0 - k) + k);
    }

    float getGeometricShadowing(void)
    {
        float geometricShadowing;
        switch (Options::BRDF::GeometricShadowing::Selection)
        {
        case Options::BRDF::GeometricShadowing::AshikhminShirley:
            geometricShadowing = getGeometricShadowingAshikhminShirley();
            break;

        case Options::BRDF::GeometricShadowing::AshikhminPremoze:
            geometricShadowing = getGeometricShadowingAshikhminPremoze();
            break;

        case Options::BRDF::GeometricShadowing::Duer:
            geometricShadowing = getGeometricShadowingDuer();
            break;

        case Options::BRDF::GeometricShadowing::Neumann:
            geometricShadowing = getGeometricShadowingNeumann();
            break;

        case Options::BRDF::GeometricShadowing::Kelemen:
            geometricShadowing = getGeometricShadowingKelemen();
            break;

        case Options::BRDF::GeometricShadowing::ModifiedKelemen:
            geometricShadowing = getGeometricShadowingModifiedKelemen();
            break;

        case Options::BRDF::GeometricShadowing::CookTorrence:
            geometricShadowing = getGeometricShadowingCookTorrence();
            break;

        case Options::BRDF::GeometricShadowing::Ward:
            geometricShadowing = getGeometricShadowingWard();
            break;

        case Options::BRDF::GeometricShadowing::Kurt:
            geometricShadowing = getGeometricShadowingKurt();
            break;

        case Options::BRDF::GeometricShadowing::WalterEtAl:
            geometricShadowing = getGeometricShadowingWalterEtAl(NdotL) * getGeometricShadowingWalterEtAl(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::Beckman:
            geometricShadowing = getGeometricShadowingBeckman(NdotL) * getGeometricShadowingBeckman(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::GGX:
            geometricShadowing = getGeometricShadowingGGX(NdotL) * getGeometricShadowingGGX(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::Schlick:
            geometricShadowing = getGeometricShadowingSchlick(NdotL) * getGeometricShadowingSchlick(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::SchlickBeckman:
            geometricShadowing = getGeometricShadowingSchlickBeckman(NdotL) * getGeometricShadowingSchlickBeckman(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::SchlickGGX:
            geometricShadowing = getGeometricShadowingSchlickGGX(NdotL) * getGeometricShadowingSchlickGGX(NdotV);
            break;

        case Options::BRDF::GeometricShadowing::Implicit:
            geometricShadowing = getGeometricShadowingImplicit();
            break;

        default:
            geometricShadowing = 0;
            break;
        };

        return geometricShadowing;
    }

    float SchlickFresnel(float i)
    {
        float x = clamp(1.0 - i, 0.0, 1.0);
        float x2 = x * x;
        return x2 * x2*x;
    }

    float3 getFresnelSchlick(void)
    {
        return specularRadiance + (1.0 - specularRadiance) * SchlickFresnel(LdotH);
    }

    float3 getFresnelSphericalGaussian(void)
    {
        float power = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        return specularRadiance + (1.0 - specularRadiance) * pow(2.0, power);
    }

    float3 getFresnel(void)
    {
        float3 fresnel;
        switch (Options::BRDF::Fresnel::Selection)
        {
        case Options::BRDF::Fresnel::Schlick:
            fresnel = getFresnelSchlick();
            break;

        case Options::BRDF::Fresnel::SphericalGaussian:
            fresnel = getFresnelSphericalGaussian();
            break;

        default:
            fresnel = 0;
            break;
        };

        return fresnel;
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

    float MixFunction(float i, float j, float x)
    {
        return  j * x + i * (1.0 - x);
    }

    float2 MixFunction(float2 i, float2 j, float x)
    {
        return  j * x + i * (1.0h - x);
    }

    float3 MixFunction(float3 i, float3 j, float x)
    {
        return  j * x + i * (1.0h - x);
    }

    float MixFunction(float4 i, float4 j, float x)
    {
        return  j * x + i * (1.0h - x);
    }

    float F0(void)
    {
        // Diffuse fresnel
        float FresnelLight = SchlickFresnel(NdotL);
        float FresnelView = SchlickFresnel(NdotV);
        float FresnelDiffuse90 = 0.5 + 2.0 * LdotH * LdotH * materialRoughness;
        return MixFunction(1.0, FresnelDiffuse90, FresnelLight) * MixFunction(1.0, FresnelDiffuse90, FresnelView);
    }
    
    float3 getIrradiance(float3 _lightDirection, float3 _lightRadiance, float _attenuation)
    {
        lightDirection = _lightDirection;
        lightReflectDirection = reflect(-lightDirection, surfaceNormal);
        lightRadiance = _lightRadiance;
        attenuation = _attenuation;
        float3 halfDirection = normalize(viewDirection + lightDirection);
        NdotL = saturate(dot(surfaceNormal, lightDirection));
        NdotH = saturate(dot(surfaceNormal, halfDirection));
        VdotH = saturate(dot(viewDirection, halfDirection));
        LdotH = saturate(dot(lightDirection, halfDirection));
        LdotV = saturate(dot(lightDirection, viewDirection));
        RdotV = saturate(dot(lightReflectDirection, viewDirection));

        diffuseRadiance = materialAlbedo * (1.0 - materialMetallic);
        float f0 = F0();
        diffuseRadiance *= f0;

        specularRadiance = lerp(lightRadiance, materialAlbedo, materialMetallic * 0.5);

        switch (Options::BRDF::Debug::Selection)
        {
        case Options::BRDF::Debug::ShowAttenuation:
            return attenuation;

        case Options::BRDF::Debug::ShowNormalDistribution:
            return getNormalDistribution();

        case Options::BRDF::Debug::ShowGeometricShadow:
            return getGeometricShadowing();

        case Options::BRDF::Debug::ShowFresnel:
            return getFresnel();
        };

        float lambert = getLambert();

        float3 specular = ((getNormalDistribution() * getGeometricShadowing() * getFresnel()) / (4.0 * NdotL * NdotV));
        /* 
            Maintain energy conservation:
            Energy conservation is a restriction on the reflection model that requires that the total amount of reflected light cannot be more than the incoming light.
            http://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
        */
        return (diffuseRadiance + specular) * lambert * attenuation;
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
    data.initialize(screenCoord, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);

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

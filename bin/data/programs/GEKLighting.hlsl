// http://www.jordanstevenstechart.com/physically-based-rendering

float getFalloff(float distance, float range)
{
    float denominator = (pow(distance, 2.0) + 1.0);
    float attenuation = pow((distance / range), 4.0);
    return (pow(saturate(1.0 - attenuation), 2.0) / denominator);
}

// schlick functions
float SchlickFresnel(float i) {
    float x = clamp(1.0 - i, 0.0, 1.0);
    float x2 = x * x;
    return x2 * x2 * x;
}

float3 FresnelLerp(float3 x, float3 y, float d)
{
    float t = SchlickFresnel(d);
    return lerp(x, y, t);
}

namespace Fresnel
{
    float3 Schlick(float3 SpecularColor, float LdotH)
    {
        return SpecularColor + (1 - SpecularColor) * SchlickFresnel(LdotH);
    }

    float3 SphericalGaussian(float LdotH, float3 SpecularColor)
    {
        float power = ((-5.55473 * LdotH) - 6.98316) * LdotH;
        return SpecularColor + (1 - SpecularColor) * pow(2, power);
    }
}; // namespace Fresnel

// normal incidence reflection calculation
float F0(float NdotL, float NdotV, float LdotH, float materialRoughness)
{
    // Diffuse fresnel
    float FresnelLight = SchlickFresnel(NdotL);
    float FresnelView = SchlickFresnel(NdotV);
    float FresnelDiffuse90 = 0.5 + 2.0 * LdotH * LdotH * materialRoughness;
    return  lerp(1, FresnelDiffuse90, FresnelLight) * lerp(1, FresnelDiffuse90, FresnelView);
}

namespace NormalDistribution
{
    float Beckmann(float materialRoughness, float NdotH)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotHSqr = NdotH * NdotH;
        return max(0.000001, (1.0 / (Math::Pi * roughnessSqr * NdotHSqr * NdotHSqr)) * exp((NdotHSqr - 1) / (roughnessSqr * NdotHSqr)));
    }

    float Gaussian(float materialRoughness, float NdotH)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float thetaH = acos(NdotH);
        return exp(-thetaH * thetaH / roughnessSqr);
    }

    float GGX(float materialRoughness, float NdotH)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotHSqr = NdotH * NdotH;
        float TanNdotHSqr = (1 - NdotHSqr) / NdotHSqr;
        return (1.0 / Math::Pi) * square(materialRoughness / (NdotHSqr * (roughnessSqr + TanNdotHSqr)));
        //    float denom = NdotHSqr * (roughnessSqr-1)
    }

    float TrowbridgeReitz(float NdotH, float materialRoughness)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float Distribution = NdotH * NdotH * (roughnessSqr - 1.0) + 1.0;
        return roughnessSqr / (Math::Pi * Distribution * Distribution);
    }
}; // namespace NormalDistribution

namespace GeometricShadowing
{
    float Implicit(float NdotL, float NdotV)
    {
        float Gs = (NdotL * NdotV);
        return Gs;
    }

    float AshikhminShirley(float NdotL, float NdotV, float LdotH)
    {
        float Gs = NdotL * NdotV / (LdotH * max(NdotL, NdotV));
        return  (Gs);
    }

    float AshikhminPremoze(float NdotL, float NdotV)
    {
        float Gs = NdotL * NdotV / (NdotL + NdotV - NdotL * NdotV);
        return  (Gs);
    }

    float Duer(float3 lightDirection, float3 viewDirection, float3 surfaceNormal, float NdotL, float NdotV)
    {
        float3 LpV = lightDirection + viewDirection;
        float Gs = dot(LpV, LpV) * pow(dot(LpV, surfaceNormal), -4);
        return  (Gs);
    }

    float Neumann(float NdotL, float NdotV)
    {
        float Gs = (NdotL * NdotV) / max(NdotL, NdotV);
        return  (Gs);
    }

    float Kelemen(float NdotL, float NdotV, float LdotH, float VdotH)
    {
        //	float Gs = (NdotL * NdotV)/ (LdotH * LdotH);           //this
        float Gs = (NdotL * NdotV) / (VdotH * VdotH);       //or this?
        return   (Gs);
    }

    float ModifiedKelemen(float NdotV, float NdotL, float materialRoughness)
    {
        float c = 0.797884560802865; // c = sqrt(2 / Pi)
        float k = materialRoughness * materialRoughness * c;
        float gH = NdotV * k + (1 - k);
        return (gH * gH * NdotL);
    }

    float CookTorrence(float NdotL, float NdotV, float VdotH, float NdotH)
    {
        float Gs = min(1.0, min(2 * NdotH * NdotV / VdotH, 2 * NdotH * NdotL / VdotH));
        return  (Gs);
    }

    float Ward(float NdotL, float NdotV, float VdotH, float NdotH)
    {
        float Gs = pow(NdotL * NdotV, 0.5);
        return  (Gs);
    }

    float Kurt(float NdotL, float NdotV, float VdotH, float alpha)
    {
        float Gs = (VdotH * pow(NdotL * NdotV, alpha)) / NdotL * NdotV;
        return  (Gs);
    }

    //SmithModelsBelow
    //Gs = F(NdotL) * F(NdotV);
    float WalterEtAl(float NdotL, float NdotV, float alpha)
    {
        float alphaSqr = alpha * alpha;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float SmithL = 2 / (1 + sqrt(1 + alphaSqr * (1 - NdotLSqr) / (NdotLSqr)));
        float SmithV = 2 / (1 + sqrt(1 + alphaSqr * (1 - NdotVSqr) / (NdotVSqr)));
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float Beckman(float NdotL, float NdotV, float materialRoughness)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float calulationL = (NdotL) / (roughnessSqr * sqrt(1 - NdotLSqr));
        float calulationV = (NdotV) / (roughnessSqr * sqrt(1 - NdotVSqr));
        float SmithL = calulationL < 1.6 ? (((3.535 * calulationL) + (2.181 * calulationL * calulationL)) / (1 + (2.276 * calulationL) + (2.577 * calulationL * calulationL))) : 1.0;
        float SmithV = calulationV < 1.6 ? (((3.535 * calulationV) + (2.181 * calulationV * calulationV)) / (1 + (2.276 * calulationV) + (2.577 * calulationV * calulationV))) : 1.0;
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float GGX(float NdotL, float NdotV, float materialRoughness)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float NdotLSqr = NdotL * NdotL;
        float NdotVSqr = NdotV * NdotV;
        float SmithL = (2 * NdotL) / (NdotL + sqrt(roughnessSqr + (1 - roughnessSqr) * NdotLSqr));
        float SmithV = (2 * NdotV) / (NdotV + sqrt(roughnessSqr + (1 - roughnessSqr) * NdotVSqr));
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float Schlick(float NdotL, float NdotV, float materialRoughness)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float SmithL = (NdotL) / (NdotL * (1 - roughnessSqr) + roughnessSqr);
        float SmithV = (NdotV) / (NdotV * (1 - roughnessSqr) + roughnessSqr);
        return (SmithL * SmithV);
    }

    float SchlickBeckman(float NdotL, float NdotV, float materialRoughness)
    {
        float roughnessSqr = materialRoughness * materialRoughness;
        float k = roughnessSqr * 0.797884560802865;
        float SmithL = (NdotL) / (NdotL * (1 - k) + k);
        float SmithV = (NdotV) / (NdotV * (1 - k) + k);
        float Gs = (SmithL * SmithV);
        return Gs;
    }

    float SchlickGGX(float NdotL, float NdotV, float materialRoughness)
    {
        float k = materialRoughness / 2;
        float SmithL = (NdotL) / (NdotL * (1 - k) + k);
        float SmithV = (NdotV) / (NdotV * (1 - k) + k);
        float Gs = (SmithL * SmithV);
        return Gs;
    }
}; // namespace GeometricShadowing

float3 getSurfaceIrradiance(
    float3 surfaceNormal, float3 viewDirection, float3 reflectedViewDirection, float NdotV,
    float3 materialAlbedo, float materialRoughness, float materialMetallic,
    float3 lightDirection, float3 lightRadiance, float attenuation)
{
    float3 lightReflectDirection = reflect(-lightDirection, surfaceNormal);
    float NdotL = max(0.0, dot(surfaceNormal, lightDirection));
    float3 halfDirection = normalize(viewDirection + lightDirection);
    float NdotH = max(0.0, dot(surfaceNormal, halfDirection));
    float VdotH = max(0.0, dot(viewDirection, halfDirection));
    float LdotH = max(0.0, dot(lightDirection, halfDirection));
    float LdotV = max(0.0, dot(lightDirection, viewDirection));
    float RdotV = max(0.0, dot(lightReflectDirection, viewDirection));
    float3 attenuatedColor = attenuation * lightRadiance;

    float3 diffuseColor = materialAlbedo * (1.0 - materialMetallic);
    float f0 = F0(NdotL, NdotV, LdotH, materialRoughness);
    diffuseColor *= f0;

    switch (Options::BRDF::Debug::Selection)
    {
    case Options::BRDF::Debug::ShowDiffuse:
        return diffuseColor;

    case Options::BRDF::Debug::ShowAttenuation:
        return attenuation;
    };


    float3 specularBase = lerp(lightRadiance, materialAlbedo, materialMetallic * 0.5);

    //Specular calculations
    float3 distribution = specularBase;
    switch (Options::BRDF::NormalDistribution::Selection)
    {
    case Options::BRDF::NormalDistribution::Beckmann:
        distribution *= NormalDistribution::Beckmann(materialRoughness, NdotH);
        break;

    case Options::BRDF::NormalDistribution::Gaussian:
        distribution *= NormalDistribution::Gaussian(materialRoughness, NdotH);
        break;

    case Options::BRDF::NormalDistribution::GGX:
        distribution *= NormalDistribution::GGX(materialRoughness, NdotH);
        break;

    case Options::BRDF::NormalDistribution::TrowbridgeReitz:
        distribution *= NormalDistribution::TrowbridgeReitz(NdotH, materialRoughness);
        break;

    default:
        distribution = 1;
        break;
    };

    float geometricShadow;
    switch (Options::BRDF::GeometricShadowing::Selection)
    {
    case Options::BRDF::GeometricShadowing::AshikhminShirley:
        geometricShadow = GeometricShadowing::AshikhminShirley(NdotL, NdotV, LdotH);
        break;

    case Options::BRDF::GeometricShadowing::AshikhminPremoze:
        geometricShadow = GeometricShadowing::AshikhminPremoze(NdotL, NdotV);
        break;

    case Options::BRDF::GeometricShadowing::Duer:
        geometricShadow = GeometricShadowing::Duer(lightDirection, viewDirection, surfaceNormal, NdotL, NdotV);
        break;

    case Options::BRDF::GeometricShadowing::Neumann:
        geometricShadow = GeometricShadowing::Neumann(NdotL, NdotV);
        break;

    case Options::BRDF::GeometricShadowing::Kelemen:
        geometricShadow = GeometricShadowing::Kelemen(NdotL, NdotV, LdotH, VdotH);
        break;

    case Options::BRDF::GeometricShadowing::ModifiedKelemen:
        geometricShadow = GeometricShadowing::ModifiedKelemen(NdotV, NdotL, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::CookTorrence:
        geometricShadow = GeometricShadowing::CookTorrence(NdotL, NdotV, VdotH, NdotH);
        break;

    case Options::BRDF::GeometricShadowing::Ward:
        geometricShadow = GeometricShadowing::Ward(NdotL, NdotV, VdotH, NdotH);
        break;

    case Options::BRDF::GeometricShadowing::Kurt:
        geometricShadow = GeometricShadowing::Kurt(NdotL, NdotV, VdotH, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::WalterEtAl:
        geometricShadow = GeometricShadowing::WalterEtAl(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::Beckman:
        geometricShadow = GeometricShadowing::Beckman(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::GGX:
        geometricShadow = GeometricShadowing::GGX(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::Schlick:
        geometricShadow = GeometricShadowing::Schlick(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::SchlickBeckman:
        geometricShadow = GeometricShadowing::SchlickBeckman(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::SchlickGGX:
        geometricShadow = GeometricShadowing::SchlickGGX(NdotL, NdotV, materialRoughness);
        break;

    case Options::BRDF::GeometricShadowing::Implicit:
        geometricShadow = GeometricShadowing::Implicit(NdotL, NdotV);
        break;

    default:
        geometricShadow = 1;
        break;
    };

    float3 fresnel;
    switch (Options::BRDF::Fresnel::Selection)
    {
    case Options::BRDF::Fresnel::Schlick:
        fresnel = Fresnel::Schlick(specularBase, LdotH);
        break;

    case Options::BRDF::Fresnel::SphericalGaussian:
        fresnel = Fresnel::SphericalGaussian(LdotH, specularBase);
        break;

    default:
        fresnel = 1;
        break;
    };

    switch (Options::BRDF::Debug::Selection)
    {
    case Options::BRDF::Debug::ShowDistribution:
        return distribution;

    case Options::BRDF::Debug::ShowFresnel:
        return fresnel;

    case Options::BRDF::Debug::ShowGeometricShadow:
        return geometricShadow;
    };

    float lambert;
    if (Options::BRDF::UseHalfLambert)
    {
        // http://developer.valvesoftware.com/wiki/Half_Lambert
        float halfLdotN = ((dot(surfaceNormal, lightDirection) * 0.5) + 0.5);
        lambert = pow(halfLdotN, 2.0);
    }
    else
    {
        lambert = NdotL;
    }

    float3 specularColor = (distribution * fresnel * geometricShadow) / (4 * (NdotL * NdotV));
    return (diffuseColor + specularColor) * lambert * attenuatedColor;
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
    float3 reflectedViewDirection = reflect(-viewDirection, surfaceNormal);
    float NdotV = saturate(dot(surfaceNormal, viewDirection));

    materialRoughness = square(materialRoughness);

    float3 surfaceIrradiance = 0.0;

    for (uint directionalIndex = 0; directionalIndex < Lights::directionalCount; directionalIndex++)
    {
        float3 lightDirection = Lights::directionalList[directionalIndex].direction;
        float3 lightRadiance = Lights::directionalList[directionalIndex].radiance;

        surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, reflectedViewDirection, NdotV, materialAlbedo, materialRoughness, materialMetallic, lightDirection, lightRadiance, 1.0);
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
        float3 centerToRay = (lightRay - (dot(lightRay, reflectedViewDirection) * reflectedViewDirection));
        float3 closestPoint = (lightRay + (centerToRay * saturate(lightData.radius / length(centerToRay))));
        float lightDistance = length(closestPoint);
        float3 lightDirection = normalize(closestPoint);
        float attenuation = getFalloff(lightDistance, lightData.range);

        surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, reflectedViewDirection, NdotV, materialAlbedo, materialRoughness, materialMetallic, lightDirection, lightData.radiance, attenuation);
    };

    while (indexOffset < spotLightEnd)
    {
        const uint lightIndex = Lights::clusterIndexList[indexOffset++];
        const Lights::SpotData lightData = Lights::spotList[lightIndex];

        float3 lightRay = (lightData.position - surfacePosition);
        float lightDistance = length(lightRay);
        float3 lightDirection = (lightRay / lightDistance);
        float rho = saturate(dot(lightData.direction, -lightDirection));
        float spotFactor = pow(saturate(rho - lightData.outerAngle) / (lightData.innerAngle - lightData.outerAngle), lightData.coneFalloff);
        float attenuation = (getFalloff(lightDistance, lightData.range) * spotFactor);

        surfaceIrradiance += getSurfaceIrradiance(surfaceNormal, viewDirection, reflectedViewDirection, NdotV, materialAlbedo, materialRoughness, materialMetallic, lightDirection, lightData.radiance, attenuation);
    };

    return surfaceIrradiance;
}

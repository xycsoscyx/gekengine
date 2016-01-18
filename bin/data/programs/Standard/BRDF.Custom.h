float getTrowbridgeReitzGGX(float materialRoughness, float NdotH)
{
    float alpha = sqr(materialRoughness);
    float alphaSquared = sqr(alpha);
    return (alphaSquared * rcp(Math::Pi * sqr(sqr(NdotH) * (alphaSquared - 1.0) + 1.0)));
}

float getMicroFacetDistribution(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    return getTrowbridgeReitzGGX(materialRoughness, NdotH);
}

float3 getSchlickFresnelApproximation(float3 F0, float VdotH)
{
    return (F0 + (1.0 - F0) * pow((1 - VdotH), 5.0));
}

float3 getFresnel(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    float3 F0 = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    return getSchlickFresnelApproximation(F0, VdotH);
}

float getSchlickBeckmann(float k, float angle)
{
    return (angle * rcp(angle * (1.0 - k) + k));
}

float getSmithGGX(float materialRoughness, float NdotL, float NdotV)
{
    float k = (sqr(materialRoughness + 1.0) * 0.125);
    return (getSchlickBeckmann(k, NdotL) * getSchlickBeckmann(k, NdotV));
}

float getGeometricAttenuation(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    return getSmithGGX(materialRoughness, NdotL, NdotV);
}

float3 getSpecularBRDF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotV = dot(surfaceNormal, viewDirection);
    float NdotH = dot(surfaceNormal, halfAngle);
    float VdotH = dot(viewDirection, halfAngle);
    float D = getMicroFacetDistribution(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    float3 F = getFresnel(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    float G = getGeometricAttenuation(materialAlbedo, materialRoughness, materialMetalness, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    return ((D * F * G) * rcp(4.0 * NdotL * NdotV));
}

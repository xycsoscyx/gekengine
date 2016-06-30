float getTrowbridgeReitzGGX(float materialRoughness, float NdotH)
{
    float alpha = square(materialRoughness);
    float alphaSquared = square(alpha);
    return (alphaSquared * rcp(Math::Pi * square(square(NdotH) * (alphaSquared - 1.0) + 1.0)));
}

float3 getSchlickFresnelApproximation(float3 materialAlbedo, float materialMetalness, float VdotH)
{
    float3 F0 = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    return (F0 + (1.0 - F0) * pow((1 - VdotH), 5.0));
}

float getSchlickBeckmann(float k, float angle)
{
    return (angle * rcp(angle * (1.0 - k) + k));
}

float getSmithGGX(float materialRoughness, float NdotL, float NdotV)
{
    float k = (square(materialRoughness + 1.0) * 0.125);
    return (getSchlickBeckmann(k, NdotL) * getSchlickBeckmann(k, NdotV));
}

float3 getSpecularBRDF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotV = dot(surfaceNormal, viewDirection);
    float NdotH = dot(surfaceNormal, halfAngle);
    float VdotH = dot(viewDirection, halfAngle);
    float D = getTrowbridgeReitzGGX(materialRoughness, NdotH);
    float3 F = getSchlickFresnelApproximation(materialAlbedo, materialMetalness, VdotH);
    float G = getSmithGGX(materialRoughness, NdotL, NdotV);
    return ((D * F * G) * rcp(4.0 * NdotL * NdotV));
}

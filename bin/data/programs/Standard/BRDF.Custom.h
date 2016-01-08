float3 getTrowbridgeReitz(float alpha, float NdotH)
{
    float alphaSquared = sqr(alpha);
    return (alphaSquared / (Math::Pi * sqr(sqr(NdotH) * (alphaSquared - 1.0f) + 1.0f)));
}

float3 getD(float3 materialAlbedo, float materialRoughness, float materialMetalness, float alpha, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    return getTrowbridgeReitz(alpha, NdotH);
}

float3 getSchlickFresnel(float3 F0, float VdotH)
{
    return (F0 + (1 - F0) * pow((1 - VdotH), 5));
}

float3 getSchlickFresnelApproximation(float3 F0, float VdotH)
{
    static const float MagicExponent = 8.656170f; // == 1/ln(2) * 6   (6 is SpecularPower of 5 + 1)
    return F0 + (1.0f - F0) * exp2(-MagicExponent * VdotH);
}

float3 getF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float alpha, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    float3 F0 = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    return getSchlickFresnelApproximation(F0, VdotH);
    return getSchlickFresnel(F0, VdotH);
}

float getG1V(float k, float angle)
{
    return (angle / (angle * (1.0f - k) + k));
}

float getSchlickSmith(float materialRoughness, float NdotL, float NdotV)
{
    float k = (sqr(materialRoughness + 1.0f) / 8.0f);
    return (getG1V(k, NdotL) * getG1V(k, NdotV));
}

float3 getG(float3 materialAlbedo, float materialRoughness, float materialMetalness, float alpha, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float3 halfAngle, float NdotL, float NdotV, float NdotH, float VdotH)
{
    const float anisotropic = 1.0f;

    float aspect = sqrt(1 - (anisotropic * 0.9f));
    float materialRoughnessSquared = (materialRoughness * materialRoughness);
    float materialRoughnessX = max(0.001f, materialRoughnessSquared / aspect);
    float materialRoughnessY = max(0.001f, materialRoughnessSquared * aspect);
    return getSchlickSmith(materialRoughnessX, NdotL, NdotV) * 
           getSchlickSmith(materialRoughnessY, NdotL, NdotV);
}

float3 getBRDF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    float alpha = (materialRoughness * materialRoughness);

    float3 diffuseColor = lerp(materialAlbedo, 0.0, materialMetalness);

    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotV = dot(surfaceNormal, viewDirection);
    float NdotH = dot(surfaceNormal, halfAngle);
    float VdotH = dot(viewDirection, halfAngle);
    float D = getD(materialAlbedo, materialRoughness, materialMetalness, alpha, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    float F = getF(materialAlbedo, materialRoughness, materialMetalness, alpha, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    float G = getG(materialAlbedo, materialRoughness, materialMetalness, alpha, surfaceNormal, lightDirection, viewDirection, halfAngle, NdotL, NdotV, NdotH, VdotH);
    return (Math::ReciprocalPi * diffuseColor) + ((D * F * G) / (4.0f * NdotL * NdotV));
}

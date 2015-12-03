// UE4 Lighting Equation

float getFresnelSchlick(float f0, float VdotH)
{
    return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

float getFresnelSchlick(float VdotH)
{
    float value = saturate(1 - VdotH);
    float value2 = value * value;
    return (value2 * value2 * value);
}

float getGGX(float alpha, float NdotH)
{
    float alpha2 = alpha * alpha;
    float t = 1 + (alpha2 - 1) * NdotH * NdotH;
    return Math::ReciprocalPi * alpha2 / (t * t);
}

float getSchlickGGX(float alpha, float NdotV)
{
    float k = 0.5 * alpha;
    return NdotV / (NdotV * (1 - k) + k);
}

float3 getBRDF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    materialRoughness = (materialRoughness * 0.9f + 0.1f);

    float3 diffuseColor = lerp(materialAlbedo, 0.0, materialMetalness);

    float3 halfAngle = normalize(lightDirection + viewDirection);
    float3 specularColor = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    float VdotH = dot(viewDirection, halfAngle);
    float fresnel = getFresnelSchlick(VdotH);
    float3 F0 = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    specularColor = lerp(F0, 1.0, fresnel);

    float alpha = sqr(materialRoughness);

    float NdotH = clamp(dot(surfaceNormal, halfAngle), 0.0001f, 1.0f);
    float D = getGGX(alpha, NdotH);

    float NdotV = clamp(dot(surfaceNormal, viewDirection), 0.0001f, 1.0f);
    float G = getSchlickGGX(alpha, NdotV);

    return (Math::ReciprocalPi * diffuseColor) + (specularColor * (0.25 * D * G) / (NdotL * NdotV));
}

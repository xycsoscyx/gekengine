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

float getSchlickGGX(float alpha, float NdotV)
{
    float k = 0.5 * alpha;
    return NdotV / (NdotV * (1 - k) + k);
}

float getGGX(float alpha, float NdotH)
{
    float alpha2 = alpha * alpha;
    float t = 1 + (alpha2 - 1) * NdotH * NdotH;
    return Math::ReciprocalPi * alpha2 / (t * t);
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    float3 diffuseColor = lerp(materialAlbedo, 0.0, materialMetalness);

    float NdotV = dot(pixelNormal, viewNormal);
    float NdotL = dot(pixelNormal, lightNormal);

    float3 H = normalize(lightNormal + viewNormal);
    float3 specularColor = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    float VdotH = dot(viewNormal, H);
    float fresnel = getFresnelSchlick(VdotH);
    float3 F0 = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
    specularColor = lerp(F0, 1.0, fresnel);

    float alpha = saturate(materialRoughness * materialRoughness);

    float NdotH = dot(pixelNormal, H);
    float D = getGGX(alpha, NdotH);

    float G = getSchlickGGX(alpha, NdotV);

    return (Math::ReciprocalPi * diffuseColor) + (specularColor * (0.25 * D * G) / (NdotL * NdotV));
}

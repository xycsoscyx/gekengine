// Simple Lighting Equation

static const float specularPower = 1000;

float3 reflect(float3 incidentNormal, float3 surfaceNormal)
{
    return 2 * dot(incidentNormal, surfaceNormal) * surfaceNormal - incidentNormal;
}

float getPhong(in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL)
{
    float3 reflectNormal = reflect(lightDirection, surfaceNormal);
    return pow(max(0, dot(reflectNormal, viewDirection)), specularPower);
}

float getGaussian(in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL)
{
    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotH = clamp(dot(surfaceNormal, halfAngle), 0, 1);
    float Threshold = 0.04;
    float CosAngle = pow(Threshold, 1 / specularPower);
    float NormAngle = (NdotH - 1) / (CosAngle - 1);
    return exp(-NormAngle * NormAngle);
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL, in float NdotV)
{
    return (materialAlbedo + getPhong(surfaceNormal, lightDirection, viewDirection, NdotL));
}

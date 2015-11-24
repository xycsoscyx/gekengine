// Simple Lighting Equation

static const float specularPower = 1000;
static const float gaussianThreshold = 0.04;

float getPhong(in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL)
{
    // Using Blinn half angle modification for performance over correctness
    float3 halfAngle = normalize(viewDirection + lightDirection);
    return pow(saturate(dot(halfAngle, surfaceNormal)), specularPower);
}

float getGaussian(in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL)
{
    float3 halfAngle = normalize(viewDirection + lightDirection);
    float NdotH = saturate(dot(surfaceNormal, halfAngle));
    float cosAngle = pow(gaussianThreshold, 1 / specularPower);
    float normalAngle = (NdotH - 1) / (cosAngle - 1);
    return exp(-normalAngle * normalAngle);
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL, in float NdotV)
{
    return (Math::ReciprocalPi * materialAlbedo) + getGaussian(surfaceNormal, lightDirection, viewDirection, NdotL);
}

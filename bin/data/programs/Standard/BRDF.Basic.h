// Simple Lighting Equation

static const float specularPower = 1000;
static const float gaussianThreshold = 0.04;

float getPhong(float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    // Using Blinn half angle modification for performance over correctness
    float3 halfAngle = normalize(viewDirection + lightDirection);
    return pow(saturate(dot(halfAngle, surfaceNormal)), specularPower);
}

float getGaussian(float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    float3 halfAngle = normalize(viewDirection + lightDirection);
    float NdotH = saturate(dot(surfaceNormal, halfAngle));
    float cosAngle = pow(gaussianThreshold, 1 / specularPower);
    float normalAngle = (NdotH - 1) / (cosAngle - 1);
    return exp(-normalAngle * normalAngle);
}

float3 getBRDF(float3 materialAlbedo, float materialRoughness, float materialMetalness, float3 surfaceNormal, float3 lightDirection, float3 viewDirection, float NdotL)
{
    float materialSmoothness = (1.0 - materialRoughness);
    return (Math::ReciprocalPi * materialAlbedo) + (getGaussian(surfaceNormal, lightDirection, viewDirection, NdotL) * materialSmoothness);
}

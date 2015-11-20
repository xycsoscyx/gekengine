// Simple Lighting Equation

static const float specularPower = 100;

float getLambert(in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    return dot(lightNormal, pixelNormal);
}

float3 reflect(float3 incidentNormal, float3 pixelNormal)
{
    return 2 * dot(incidentNormal, pixelNormal) * pixelNormal - incidentNormal;
}

float getPhong(in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    float3 reflectNormal = reflect(lightNormal, pixelNormal);
    float specular = pow(max(0, dot(reflectNormal, viewNormal)), specularPower);
    if (true)
    {
        specular = specular / dot(pixelNormal, lightNormal);
    }

    return specular;
}

float getGaussian(in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    float3 halfAngle = normalize(lightNormal + viewNormal);
    float NdotH = clamp(dot(pixelNormal, halfAngle), 0, 1);
    float Threshold = 0.04;
    float CosAngle = pow(Threshold, 1 / specularPower);
    float NormAngle = (NdotH - 1) / (CosAngle - 1);
    float specular = exp(-NormAngle * NormAngle);
    if (true)
    {
        specular *= 0.17287429 + 0.01388682 * specularPower;
    }

    return specular;
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    return ((materialAlbedo * getLambert(pixelNormal, lightNormal, viewNormal)) + getGaussian(pixelNormal, lightNormal, viewNormal));
}

// Diffuse & Specular Term
// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
float3 getBRDF(in float3 materialAlbedo, in float2 materialInfo, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    float materialSpecular = 0.3;
    float materialRoughness = materialInfo.x;
    float materialRoughnessSquared = (materialRoughness * materialRoughness);
    float materialMetalness = materialInfo.y;

    float3 Ks = lerp(materialSpecular, materialAlbedo, materialMetalness);
    float3 Kd = lerp(materialAlbedo, 0, materialMetalness);
    float3 Fd = 1.0 - Ks;

    float angleCenterView = dot(pixelNormal, viewNormal);
    float angleCenterViewSquared = (angleCenterView * angleCenterView);
    float inverseAngleCenterViewSquared = (1.0 - angleCenterViewSquared);

    float3 halfVector = normalize(lightNormal + viewNormal);
    float angleCenterHalf = saturate(dot(pixelNormal, halfVector));
    float angleLightHalf = saturate(dot(lightNormal, halfVector));

    float angleCenterLight = saturate(dot(pixelNormal, lightNormal));
    float angleCenterLightSquared = (angleCenterLight * angleCenterLight);
    float angleCenterHalfSquared = (angleCenterHalf * angleCenterHalf);
    float inverseAngleCenterLightSquared = (1.0 - angleCenterLightSquared);

    float3 diffuseContribution = (Kd * Fd * Math::ReciprocalPi * saturate(((1 - materialRoughness) * 0.5) + 0.5 + (materialRoughnessSquared * (8 - materialRoughness) * 0.023)));

    float centeredAngleCenterView = ((angleCenterView * 0.5) + 0.5);
    float centeredAngleCenterLight = ((angleCenterLight * 0.5) + 0.5);
    angleCenterViewSquared = (centeredAngleCenterView * centeredAngleCenterView);
    angleCenterLightSquared = (centeredAngleCenterLight * centeredAngleCenterLight);
    inverseAngleCenterViewSquared = (1.0 - angleCenterViewSquared);
    inverseAngleCenterLightSquared = (1.0 - angleCenterLightSquared);

    float diffuseDelta = lerp((1 / (0.1 + materialRoughness)), (-materialRoughnessSquared * 2), saturate(materialRoughness));
    float diffuseViewAngle = (1 - (pow(1 - centeredAngleCenterView, 4) * diffuseDelta));
    float diffuseLightAngle = (1 - (pow(1 - centeredAngleCenterLight, 4) * diffuseDelta));
    diffuseContribution *= (diffuseLightAngle * diffuseViewAngle * angleCenterLight);

    float3 Fs = (Ks + (Fd * pow((1 - angleLightHalf), 5)));
    float D = (pow(materialRoughness / (angleCenterHalfSquared * (materialRoughnessSquared + (1 - angleCenterHalfSquared) / angleCenterHalfSquared)), 2) * Math::ReciprocalPi);
    float3 specularContribution = (Fs * D * angleCenterLight);

    return (diffuseContribution + specularContribution);
}

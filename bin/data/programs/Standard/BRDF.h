#define _ENABLE_DIFFUSE     1
#define _ENABLE_SPECULAR    1
#define _USE_FRESNEL        1
#define _USE_NDF            1
#define _USE_GEOM           1

float fresnelSchlick(float f0, float dot_v_h)
{
    return f0 + (1 - f0) * pow(1 - dot_v_h, 5);
}

float fresnelSchlick(float dot_v_h)
{
    float value = saturate(1 - dot_v_h);
    float value2 = value * value;
    return (value2 * value2 * value);
}

float geomSchlickGGX(float alpha, float dot_n_v)
{
    float k = 0.5 * alpha;
    float geom = dot_n_v / (dot_n_v * (1 - k) + k);
    return geom;
}

float ndfGGX(float alpha, float NdotH)
{
    float alpha2 = alpha * alpha;
    float t = 1 + (alpha2 - 1) * NdotH * NdotH;
    return Math::ReciprocalPi * alpha2 / (t * t);
}

// UE4 Lighting Equation
float3 getBRDF_ue4(in float3 materialAlbedo, in float2 materialInfo, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    float materialRoughness = materialInfo.x;
    float materialMetalness = materialInfo.y;

    float dot_n_v = dot(pixelNormal, viewNormal);
    float dot_n_l = dot(pixelNormal, lightNormal);

    #if _ENABLE_DIFFUSE
        float3 diffuse_color = Math::ReciprocalPi * (1.0 - materialMetalness) * materialAlbedo;
    #endif

    #if _ENABLE_SPECULAR
        float3  h = normalize(lightNormal + viewNormal);
        float3 specular_color = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
        #if _USE_FRESNEL
            float dot_v_h = dot(viewNormal, h);
            float fresnel = fresnelSchlick(dot_v_h);
            float3  f0_color = lerp(materialAlbedo, 1.0, (1.0 - materialMetalness));
            specular_color = lerp(f0_color, 1.0, fresnel);
        #endif

        float alpha = 1.0;
        #if (_USE_NDF || _USE_GEOM)
            alpha = saturate(materialRoughness * materialRoughness);
        #endif

        float ndf = 1.0;
        #ifdef _USE_NDF
            float dot_n_h = dot(pixelNormal, h);
            ndf = ndfGGX(alpha, dot_n_h);
        #endif

        float geom = 1.0;
        #ifdef _USE_GEOM
            geom = geomSchlickGGX(alpha, dot_n_v);
        #endif

        specular_color *= (0.25 * ndf * geom) / (dot_n_l * dot_n_v);
    #endif

    #if (_ENABLE_DIFFUSE && _ENABLE_SPECULAR)
        return (diffuse_color + specular_color);
    #elif _ENABLE_DIFFUSE
        return diffuse_color;
    #elif _ENABLE_SPECULAR
        return specular_color;
    #else
        return 0.0;
    #endif
}

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

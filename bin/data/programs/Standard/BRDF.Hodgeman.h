// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
// http://pastebin.com/m7NLvtWk

static const float materialSpecular = 0.3f;
static const float materialFacetAngle = 0.0f;
static const float materialRetroreflective = 0.0f;
static const bool materialIsotropic = true;
static const bool performHelmholtzTest = false;

static const float3 X = 0.0f;
static const float3 Y = 0.0f;

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL, in float NdotV)
{
    float materialRoughnessX = materialRoughness;
    float materialRoughnessY = materialRoughness;

    if (performHelmholtzTest)
    {
        float3 temp = lightDirection; lightDirection = viewDirection; viewDirection = temp;
    }

    float sinFacetAngle = sin(materialFacetAngle);
    float cosFacetAngle = cos(materialFacetAngle);
    float3 rX = materialIsotropic ? 0 : X * cosFacetAngle + Y * -sinFacetAngle;
    float3 rY = materialIsotropic ? 0 : X * sinFacetAngle + Y *  cosFacetAngle;

    float3 Ks = lerp(materialSpecular, materialAlbedo, materialMetalness);
    float3 Kd = lerp(materialAlbedo, 0, materialMetalness);
    float3 Fd = 1.0 - Ks;

    float NdotVSquared = NdotV * NdotV;
    float OneMinusNdotVSquared = 1.0 - NdotVSquared;

    materialRoughnessX = materialRoughnessX*materialRoughnessX * 4;
    materialRoughnessY = materialRoughnessY*materialRoughnessY * 4;
    float materialRoughnessXSquared = materialRoughnessX * materialRoughnessX;
    float materialRoughnessYSquared = materialIsotropic ? materialRoughnessXSquared : materialRoughnessY * materialRoughnessY;
    float materialRoughnessSquared = (materialRoughnessXSquared + materialRoughnessYSquared) / 2;
    materialRoughness = materialIsotropic ? materialRoughnessX : sqrt(materialRoughnessSquared);

    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotH = dot(surfaceNormal, halfAngle);
    float VdotH = dot(viewDirection, halfAngle);
    float LdotH = dot(lightDirection, halfAngle);

    float LdotV = dot(lightDirection, viewDirection);
    NdotH = lerp(NdotH, LdotV, materialRetroreflective);

    float NdotLSquared = NdotL * NdotL;
    float NdotHSquared = NdotH * NdotH;
    float OneMinusNdotLSquared = 1.0 - NdotLSquared;

    float3 Rd = Kd * Fd * Math::ReciprocalPi * saturate((1 - materialRoughness)*0.5 + 0.5 + materialRoughnessSquared*(8 - materialRoughness)*0.023);

    float D;
    if (materialIsotropic)
    {
        D = pow(materialRoughness / (NdotHSquared * (materialRoughnessSquared + (1 - NdotHSquared) / NdotHSquared)), 2) / Math::Pi;
    }
    else
    {
        float HdotX = dot(halfAngle, rX);
        float HdotY = dot(halfAngle, rY);
        float HdotXSquared = HdotX * HdotX;
        float HdotYSquared = HdotY * HdotY;
        if (materialRetroreflective > 0)
        {
            float3 LX = normalize(cross(rY, lightDirection));
            float3 LY = normalize(cross(lightDirection, LX));
            float VdotLX = dot(viewDirection, LX);
            float VdotLY = dot(viewDirection, LY);
            float VdotLXSquared = VdotLX * VdotLX;
            float VdotLYSquared = VdotLY * VdotLY;

            float3 VX = normalize(cross(rY, viewDirection));
            float3 VY = normalize(cross(viewDirection, VX));
            float LdotVX = dot(lightDirection, VX);
            float LdotVY = dot(lightDirection, VY);
            float LdotVXSquared = LdotVX * LdotVX;
            float LdotVYSquared = LdotVY * LdotVY;

            float RXSquared = (LdotVXSquared + VdotLXSquared)*0.5;
            float RYSquared = (LdotVYSquared + VdotLYSquared)*0.5;

            HdotXSquared = lerp(HdotXSquared, RXSquared, materialRetroreflective);
            HdotYSquared = lerp(HdotYSquared, RYSquared, materialRetroreflective);
        }

        D = 1.0 / (Math::Pi * materialRoughnessX * materialRoughnessY * pow(HdotXSquared / materialRoughnessXSquared + HdotYSquared / materialRoughnessYSquared + NdotHSquared, 2));
    }

    NdotV = NdotV*0.5 + 0.5;
    NdotL = NdotL*0.5 + 0.5;
    NdotLSquared = NdotL * NdotL;
    NdotVSquared = NdotV * NdotV;
    OneMinusNdotLSquared = 1.0 - NdotLSquared;
    OneMinusNdotVSquared = 1.0 - NdotVSquared;

    float diffuseF1 = pow(1 - NdotL, 4);
    float diffuseF2 = pow(1 - NdotV, 4);
    float dd = lerp((1 / (0.1 + materialRoughness)), -materialRoughnessSquared * 2, saturate(materialRoughness));
    diffuseF1 = 1 - (diffuseF1 * dd);
    diffuseF2 = 1 - (diffuseF2 * dd);
    Rd *= diffuseF1 * diffuseF2;

    float3 Fs = Ks + Fd * pow(1 - LdotH, 5);

    float G1L = 1.0 + sqrt(1.0 + materialRoughnessSquared * (OneMinusNdotLSquared / NdotLSquared));
    float G1V = 1.0 + sqrt(1.0 + materialRoughnessSquared * (OneMinusNdotVSquared / NdotVSquared));
    float G = ((2 / G1L) * (2 / G1V)) / (4 * NdotV * NdotL + 0.1);

    float retroreflective = NdotVSquared * NdotL;
    G = lerp(G, retroreflective, materialRetroreflective);

    float3 Rs = Fs * D * G;

    return Rd;
    return (Rd + Rs);
}

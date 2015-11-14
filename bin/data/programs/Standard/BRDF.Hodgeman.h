// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
// http://pastebin.com/m7NLvtWk

static const float materialRetroReflectivity = 0.0; // 0-1
static const float materialFacetAngle = 0.0; // 0-Pi
static const bool materialIsotropic = true;

float3 getBRDF(in float3 materialAlbedo, in float2 materialInfo, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    static const float X = 0.5;
    static const float Y = 0.5;

#ifdef _HELMHOLTZ_TEST
    float3 temp = lightNormal; lightNormal = viewNormal; viewNormal = temp;
#endif

    float2 materialRoughness = materialInfo.xy;
    float materialMetalness = materialInfo.z;
    float materialSpecular = 0.3;

    float sinFacetAngle = sin(materialFacetAngle);
    float cosFacetAngle = cos(materialFacetAngle);
    float3 rX = materialIsotropic ? 0.0 : X * cosFacetAngle + Y * -sinFacetAngle;
    float3 rY = materialIsotropic ? 0.0 : X * sinFacetAngle + Y *  cosFacetAngle;

    float3 Ks = lerp(materialSpecular, materialAlbedo, materialMetalness);
    float3 Kd = lerp(materialAlbedo, 0.0, materialMetalness);
    float3 Fd = 1.0 - Ks;

    float NdotV = dot(pixelNormal, viewNormal);
    float NdotVSquared = NdotV * NdotV;
    float OneMinusNdotVSquared = 1.0 - NdotVSquared;

    float materialRoughnessX = materialRoughness.x;
    float materialRoughnessY = materialRoughness.y;
    materialRoughnessX = materialRoughnessX*materialRoughnessX * 4;
    materialRoughnessY = materialRoughnessY*materialRoughnessY * 4;
    float materialRoughnessXSquared = materialRoughnessX * materialRoughnessX;
    float materialRoughnessYSquared = materialIsotropic ? materialRoughnessXSquared : materialRoughnessY * materialRoughnessY;
    float materialRoughnessSquared = (materialRoughnessXSquared + materialRoughnessYSquared) / 2;
    materialRoughness = materialIsotropic ? materialRoughnessX : sqrt(materialRoughnessSquared);

    float3 H = normalize(lightNormal + viewNormal);
    float NdotL = saturate(dot(pixelNormal, lightNormal));
    float NdotH = saturate(dot(pixelNormal, H));
    float VdotH = dot(viewNormal, H);
    float LdotH = saturate(dot(lightNormal, H));

    float LdotV = dot(lightNormal, viewNormal);
    NdotH = lerp(NdotH, LdotV, saturate(materialRetroReflectivity));

    float NdotLSquared = NdotL * NdotL;
    float NdotHSquared = NdotH * NdotH;
    float OneMinusNdotLSquared = 1.0 - NdotLSquared;

    float3 Rd = Kd * Fd * 1 / Math::Pi * saturate((1 - materialRoughness) * 0.5 + 0.5 + materialRoughnessSquared * (8 - materialRoughness) * 0.023);

    float D;
    if (materialIsotropic)
    {
        D = pow(materialRoughness / (NdotHSquared * (materialRoughnessSquared + (1 - NdotHSquared) / NdotHSquared)), 2) / Math::Pi;
    }
    else
    {
        float HdotX = dot(H, rX);
        float HdotY = dot(H, rY);
        float HdotXSquared = HdotX * HdotX;
        float HdotYSquared = HdotY * HdotY;
        if (materialRetroReflectivity > 0)
        {
            float3 LX = normalize(cross(rY, lightNormal));
            float3 LY = normalize(cross(lightNormal, LX));
            float VdotLX = dot(viewNormal, LX);
            float VdotLY = dot(viewNormal, LY);
            float VdotLXSquared = VdotLX * VdotLX;
            float VdotLYSquared = VdotLY * VdotLY;

            float3 VX = normalize(cross(rY, viewNormal));
            float3 VY = normalize(cross(viewNormal, VX));
            float LdotVX = dot(lightNormal, VX);
            float LdotVY = dot(lightNormal, VY);
            float LdotVXSquared = LdotVX * LdotVX;
            float LdotVYSquared = LdotVY * LdotVY;

            float RXSquared = (LdotVXSquared + VdotLXSquared) * 0.5;
            float RYSquared = (LdotVYSquared + VdotLYSquared) * 0.5;

            HdotXSquared = lerp(HdotXSquared, RXSquared, materialRetroReflectivity);
            HdotYSquared = lerp(HdotYSquared, RYSquared, materialRetroReflectivity);
        }

        D = 1.0 / (Math::Pi * materialRoughnessX * materialRoughnessY * pow(HdotXSquared / materialRoughnessXSquared + HdotYSquared / materialRoughnessYSquared + NdotHSquared, 2));
    }

    NdotV = NdotV * 0.5 + 0.5;
    NdotL = NdotL * 0.5 + 0.5;
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

    float G1NdotL = 1.0 + sqrt(1.0 + materialRoughnessSquared * (OneMinusNdotLSquared / NdotLSquared));
    float G1NdotV = 1.0 + sqrt(1.0 + materialRoughnessSquared * (OneMinusNdotVSquared / NdotVSquared));
    float G = ((2 / G1NdotL) * (2 / G1NdotV)) / (4 * NdotV * NdotL + 0.1);

    G = lerp(G, (NdotVSquared * NdotL), materialRetroReflectivity);

    float3 Rs = Fs * D * G;

    return float3(Rd + Rs);
}

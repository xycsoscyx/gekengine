// http://www.gamedev.net/topic/639226-your-preferred-or-desired-brdf/
// http://pastebin.com/m7NLvtWk

static const float materialFacetAngle = 0.0f;
static const float materialRetroreflective = 0.0f;
static const bool materialIsotropic = false;
static const bool performHelmholtzTest = false;
static const float3 X = 0.5f;
static const float3 Y = 0.5f;

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 pixelNormal, in float3 lightNormal, in float3 viewNormal)
{
    static const float materialSpecular = 0.3f;
    float materialRoughnessX = materialRoughness;
    float materialRoughnessY = materialRoughness;

    if (performHelmholtzTest)
    {
        float3 temp = lightNormal; lightNormal = viewNormal; viewNormal = temp;
    }

    float sina = sin(materialFacetAngle);
    float cosa = cos(materialFacetAngle);
    float3 rX = materialIsotropic ? 0 : X * cosa + Y * -sina;
    float3 rY = materialIsotropic ? 0 : X * sina + Y *  cosa;

    float3 Ks = lerp(materialSpecular, materialAlbedo, materialMetalness);
    float3 Kd = lerp(materialAlbedo, 0, materialMetalness);
    float3 Fd = 1.0 - Ks;

    float NdotV = dot(pixelNormal, viewNormal);
    float NdotV_2 = NdotV * NdotV;
    float OneMinusNdotV_2 = 1.0 - NdotV_2;

    float M, M_2, Mx_2, My_2;
    float MX = materialRoughnessX;
    float MY = materialRoughnessY;
    MX = MX*MX * 4;
    MY = MY*MY * 4;
    Mx_2 = MX * MX;
    My_2 = materialIsotropic ? Mx_2 : MY * MY;
    M_2 = (Mx_2 + My_2) / 2;
    M = materialIsotropic ? MX : sqrt(M_2);

    float3 H = normalize(lightNormal + viewNormal);
    float NdotL = saturate(dot(pixelNormal, lightNormal));
    float NdotH = saturate(dot(pixelNormal, H));
    float VdotH = dot(viewNormal, H);
    float LdotH = saturate(dot(lightNormal, H));

    float LdotV = dot(lightNormal, viewNormal);
    NdotH = lerp(NdotH, LdotV, saturate(materialRetroreflective));

    float NdotL_2 = NdotL * NdotL;
    float NdotH_2 = NdotH * NdotH;
    float OneMinusNdotL_2 = 1.0 - NdotL_2;

    float3 Rd = Kd * Fd * Math::ReciprocalPi * saturate((1 - M)*0.5 + 0.5 + M_2*(8 - M)*0.023);

    float D;
    if (materialIsotropic)
    {
        D = pow(M / (NdotH_2 * (M_2 + (1 - NdotH_2) / NdotH_2)), 2) / Math::Pi;
    }
    else
    {
        float HdotX = dot(H, rX);
        float HdotY = dot(H, rY);
        float HdotX_2 = HdotX * HdotX;
        float HdotY_2 = HdotY * HdotY;
        if (materialRetroreflective > 0)
        {
            float3 LX = normalize(cross(rY, lightNormal));
            float3 LY = normalize(cross(lightNormal, LX));
            float VdotLX = dot(viewNormal, LX);
            float VdotLY = dot(viewNormal, LY);
            float VdotLX_2 = VdotLX * VdotLX;
            float VdotLY_2 = VdotLY * VdotLY;

            float3 VX = normalize(cross(rY, viewNormal));
            float3 VY = normalize(cross(viewNormal, VX));
            float LdotVX = dot(lightNormal, VX);
            float LdotVY = dot(lightNormal, VY);
            float LdotVX_2 = LdotVX * LdotVX;
            float LdotVY_2 = LdotVY * LdotVY;

            float RX_2 = (LdotVX_2 + VdotLX_2)*0.5;
            float RY_2 = (LdotVY_2 + VdotLY_2)*0.5;

            HdotX_2 = lerp(HdotX_2, RX_2, materialRetroreflective);
            HdotY_2 = lerp(HdotY_2, RY_2, materialRetroreflective);
        }
        D = 1.0 / (Math::Pi * MX * MY * pow(HdotX_2 / Mx_2 + HdotY_2 / My_2 + NdotH_2, 2));
    }

    NdotV = NdotV*0.5 + 0.5;
    NdotL = NdotL*0.5 + 0.5;
    NdotL_2 = NdotL * NdotL;
    NdotV_2 = NdotV * NdotV;
    OneMinusNdotL_2 = 1.0 - NdotL_2;
    OneMinusNdotV_2 = 1.0 - NdotV_2;

    float diffuseF1 = pow(1 - NdotL, 4);
    float diffuseF2 = pow(1 - NdotV, 4);
    float dd = lerp((1 / (0.1 + M)), -M_2 * 2, saturate(M));
    diffuseF1 = 1 - (diffuseF1 * dd);
    diffuseF2 = 1 - (diffuseF2 * dd);
    Rd *= diffuseF1 * diffuseF2;

    float3 Fs = Ks + Fd * pow(1 - LdotH, 5);

    float G1_1 = 1.0 + sqrt(1.0 + M_2 * (OneMinusNdotL_2 / NdotL_2));
    float G1_2 = 1.0 + sqrt(1.0 + M_2 * (OneMinusNdotV_2 / NdotV_2));
    float G = ((2 / G1_1) * (2 / G1_2)) / (4 * NdotV * NdotL + 0.1);

    float G_Retro = NdotV_2 * NdotL;
    G = lerp(G, G_Retro, materialRetroreflective);

    float3 Rs = Fs * D * G;

    return (Rd + Rs);
}

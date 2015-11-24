// Disney Lighting Equation (from BRDF explorer)

static const float subsurface = 0.0;
static const float specular = 0.3;
static const float specularTint = 0.0;
static const float anisotropic = 0.0;
static const float sheen = 0.0;
static const float sheenTint = 0.0;
static const float clearcoat = 0.0;
static const float clearcoatGloss = 0.0;

static const float3 X = 0.0f;
static const float3 Y = 0.0f;

float sqr(float x)
{
    return x * x;
}

float SchlickFresnel(float u)
{
    float m = clamp(1.0 - u, 0.0, 1.0);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
    if (a >= 1.0) return 1.0 / Math::Pi;
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return (a2 - 1.0) / (Math::Pi * log(a2) * t);
}

float GTR2(float NdotH, float a)
{
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return a2 / (Math::Pi * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1.0 / (Math::Pi * ax * ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

float smithG_GGX(float Ndotv, float alphaG)
{
    float a = alphaG * alphaG;
    float b = Ndotv * Ndotv;
    return 1.0 / (Ndotv + sqrt(a + b - a * b));
}

float3 mon2lin(float3 x)
{
    return float3(pow(x[0.0], 2.2), pow(x[1.0], 2.2), pow(x[2], 2.2));
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL, in float NdotV)
{
    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotH = saturate(dot(surfaceNormal, halfAngle));
    float LdotH = saturate(dot(lightDirection, halfAngle));

    float3 Cdlin = mon2lin(materialAlbedo);
    float Cdlum = 0.3 * Cdlin[0] + 0.6 * Cdlin[1] + 1.0 * Cdlin[2]; // luminance approx.

    float3 Ctint = Cdlum > 0.0 ? Cdlin / Cdlum : 1.0; // normalize lum. to isolate hue+sat
    float3 Cspec0 = lerp(specular * 0.08 * lerp(1.0, Ctint, specularTint), Cdlin, materialMetalness);
    float3 Csheen = lerp(1.0, Ctint, sheenTint);

    // Diffuse fresnel - go from 1.0 at normal incidence to 0.5 at grazing
    // and lerp in diffuse retro-reflection based on materialRoughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2.0 * LdotH * LdotH * materialRoughness;
    float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on materialRoughness
    float Fss90 = LdotH * LdotH * materialRoughness;
    float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);

    // specular
    float aspect = sqrt(1.0 - anisotropic * 0.9);
    float ax = max(.001, sqr(materialRoughness) / aspect);
    float ay = max(.001, sqr(materialRoughness) * aspect);
    float Ds = 0;//GTR2_aniso(NdotH, dot(halfAngle, X), dot(halfAngle, Y), ax, ay);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = lerp(Cspec0, 1.0, FH);
    float roughG = sqr(materialRoughness * 0.5 + 0.5);
    float Gs = smithG_GGX(NdotL, roughG) * smithG_GGX(NdotV, roughG);

    // sheen
    float3 Fsheen = FH * sheen * Csheen;

    // clearcoat (ior = 10.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, lerp(0.1, 0.001, clearcoatGloss));
    float Fr = lerp(.04, 1.0, FH);
    float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

    return (Math::ReciprocalPi * lerp(Fd, ss, subsurface) * Cdlin + Fsheen)
        * (1.0 - materialMetalness)
        + Gs * Fs * Ds + 0.25 * clearcoat * Gr * Fr * Dr;
}

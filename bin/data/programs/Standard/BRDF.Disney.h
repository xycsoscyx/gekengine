// Disney Lighting Equation (from BRDF explorer)

static const float materialSubsurface = 0;
static const float materialSpecular = 0;
static const float materialSpecularTint = 0;
static const float materialAnisotropic = 0;

static const float3 X = 0.0f;
static const float3 Y = 0.0f;

float sqrt(float x)
{
    return x * x;
}

float SchlickFresnel(float u)
{
    float m = clamp(1 - u, 0, 1);
    float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
    if (a >= 1) return Math::ReciprocalPi;
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return (a2 - 1) / (Math::Pi * log(a2) * t);
}

float GTR2(float NdotH, float a)
{
    float a2 = a * a;
    float t = 1 + (a2 - 1) * NdotH * NdotH;
    return a2 / (Math::Pi * t * t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1 / (Math::Pi * ax * ay * sqrt(sqrt(HdotX / ax) + sqrt(HdotY / ay) + NdotH * NdotH));
}

float smithG_GGX(float Ndotv, float alphaG)
{
    float a = alphaG * alphaG;
    float b = Ndotv * Ndotv;
    return 1 / (Ndotv + sqrt(a + b - a * b));
}

float3 mon2lin(float3 x)
{
    return float3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL, in float NdotV)
{
    float3 halfNormal = normalize(lightDirection + viewDirection);
    float NdotH = dot(surfaceNormal, halfNormal);
    float LdotH = dot(lightDirection, halfNormal);

    float3 Cdlin = mon2lin(materialAlbedo);
    float Cdlum = .3 * Cdlin[0] + .6 * Cdlin[1] + .1 * Cdlin[2]; // luminance approx.

    float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : 1; // normalize lum. to isolate hue+sat
    float3 Cspec0 = lerp(materialSpecular * .08 * lerp(1, Ctint, materialSpecularTint), Cdlin, materialMetalness);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and lerp in diffuse retro-reflection based on materialRoughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2 * LdotH * LdotH * materialRoughness;
    float Fd = lerp(1, Fd90, FL) * lerp(1, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on materialRoughness
    float Fss90 = LdotH * LdotH * materialRoughness;
    float Fss = lerp(1, Fss90, FL) * lerp(1, Fss90, FV);
    float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - .5) + .5);

    // materialSpecular
    float aspect = sqrt(1 - materialAnisotropic * .9);
    float ax = max(.001, sqrt(materialRoughness) / aspect);
    float ay = max(.001, sqrt(materialRoughness) * aspect);
    float Ds = GTR2_aniso(NdotH, dot(halfNormal, X), dot(halfNormal, Y), ax, ay);
    float FH = SchlickFresnel(LdotH);
    float3 Fs = lerp(Cspec0, 1, FH);
    float roughg = sqrt(materialRoughness * .5 + .5);
    float Gs = smithG_GGX(NdotL, roughg) * smithG_GGX(NdotV, roughg);

    return (Math::ReciprocalPi * lerp(Fd, ss, materialSubsurface) * Cdlin)
        * (1 - materialMetalness)
        + Gs * Fs * Ds;
}

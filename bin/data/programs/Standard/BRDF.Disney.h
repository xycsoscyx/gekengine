// Disney Lighting Equation (from BRDF explorer)

static const float subsurface = 0;
static const float specular = 1;
static const float specularTint = 0;
static const float anisotropic = 0;
static const float sheen = 0;
static const float sheenTint = 0;
static const float clearcoat = 0;
static const float clearcoatGloss = 0;

static const float3 X = 0;
static const float3 Y = 0;

float SchlickFresnel(float u)
{
    float m = clamp(1 - u, 0, 1);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

float GTR1(float NdotH, float alpha)
{
    if (alpha >= 1) return 1 / Math::Pi;
    float alphaSquared = alpha*alpha;
    float t = 1 + (alphaSquared - 1)*NdotH*NdotH;
    return (alphaSquared - 1) / (Math::Pi*log(alphaSquared)*t);
}

float GTR2(float NdotH, float alpha)
{
    float alphaSquared = alpha*alpha;
    float t = 1 + (alphaSquared - 1)*NdotH*NdotH;
    return alphaSquared / (Math::Pi * t*t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
    return 1 / (Math::Pi * ax*ay * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH*NdotH));
}

float smithG_GGX(float angle, float alpha)
{
    float alphaSquared = alpha*alpha;
    float angleSquared = angle*angle;
    return 1 / (angle + sqrt(alphaSquared + angleSquared - alphaSquared*angleSquared));
}

float3 getBRDF(in float3 materialAlbedo, in float materialRoughness, in float materialMetalness, in float3 surfaceNormal, in float3 lightDirection, in float3 viewDirection, in float NdotL)
{
    materialRoughness = (materialRoughness * 0.9f + 0.1f);

    float NdotV = dot(surfaceNormal, viewDirection);

    float3 halfAngle = normalize(lightDirection + viewDirection);
    float NdotH = dot(surfaceNormal, halfAngle);
    float LdotH = dot(lightDirection, halfAngle);

    float3 Cdlin = materialAlbedo;
    static const float3 luminance = float3(0.299f, 0.587f, 0.114f);
    float Cdlum = dot(Cdlin, luminance);

    float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : 1; // normalize lum. to isolate hue+sat
    float3 Cspec0 = lerp(specular*.08*lerp(1, Ctint, specularTint), Cdlin, materialMetalness);
    float3 Csheen = lerp(1, Ctint, sheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and lerp in diffuse retro-reflection based on materialRoughness
    float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
    float Fd90 = 0.5 + 2 * LdotH*LdotH * materialRoughness;
    float Fd = lerp(1, Fd90, FL) * lerp(1, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on materialRoughness
    float Fss90 = LdotH*LdotH*materialRoughness;
    float Fss = lerp(1, Fss90, FL) * lerp(1, Fss90, FV);
    float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - .5) + .5);

#define _GTR2 1

    // specular
#if _ANISOTROPIC
    float aspect = sqrt(1 - anisotropic*.9);
    float ax = max(.001, sqr(materialRoughness) / aspect);
    float ay = max(.001, sqr(materialRoughness) * aspect);
    float Ds = GTR2_aniso(NdotH, dot(halfAngle, X), dot(halfAngle, Y), ax, ay);
#elif _GTR2
    float Ds = GTR2(NdotH, sqr(materialRoughness));
#else
    float Ds = GTR1(NdotH, sqr(materialRoughness));
#endif

    float FH = SchlickFresnel(LdotH);
    float3 Fs = lerp(Cspec0, 1, FH);

    float alpha = sqr(materialRoughness);
    float Gs = smithG_GGX(NdotL, alpha) * smithG_GGX(NdotV, alpha);

    // sheen
    float3 Fsheen = FH * sheen * Csheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NdotH, lerp(.1, .001, clearcoatGloss));
    float Fr = lerp(.04, 1, FH);
    float Gr = smithG_GGX(NdotL, .25) * smithG_GGX(NdotV, .25);

    return ((1 / Math::Pi) * lerp(Fd, ss, subsurface)*Cdlin + Fsheen) * (1 - materialMetalness)
        + Gs*Fs*Ds
        + .25*clearcoat*Gr*Fr*Dr;
}

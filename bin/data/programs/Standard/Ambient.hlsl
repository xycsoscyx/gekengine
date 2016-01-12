#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float AmbientLevel = 1.0;

static const float3 SkyColor = float3((88.0 / 255.0), (58.0 / 255.0), (129.0 / 25.0));
static const float3 HorizonColor = float3(0, 0, 0);
static const float3 GroundColor = float3((224.0 / 255.0), (252.0 / 255.0), (255.0 / 255.0));

// http://www.gamedev.net/topic/673746-gradient-ambient-in-unity-how-is-it-achieved/
float3 getHemisphericalAmbient(float3 surfaceNormal)
{
    float up = saturate(surfaceNormal.y);
    float down = saturate(-surfaceNormal.y);
    float middle = 1.0 - abs(surfaceNormal.y);

    return (GroundColor * up + SkyColor * down + HorizonColor * middle);
}

static const float CosineA0 = 1.0;
static const float CosineA1 = 2.0 / 3.0;
static const float CosineA2 = 0.25;

struct SH9
{
    float coefficients[9];
};

struct SH9Color
{
    float3 coefficients[9];
};

SH9 ProjectOntoSH9(float3 dir)
{
    SH9 sh;

    // Band 0
    sh.coefficients[0] = 0.282095;

    // Band 1
    sh.coefficients[1] = 0.488603 * dir.y;
    sh.coefficients[2] = 0.488603 * dir.z;
    sh.coefficients[3] = 0.488603 * dir.x;

    // Band 2
    sh.coefficients[4] = 1.092548 * dir.x * dir.y;
    sh.coefficients[5] = 1.092548 * dir.y * dir.z;
    sh.coefficients[6] = 0.315392 * (3.0 * dir.z * dir.z - 1.0);
    sh.coefficients[7] = 1.092548 * dir.x * dir.z;
    sh.coefficients[8] = 0.546274 * (dir.x * dir.x - dir.y * dir.y);

    return sh;
}

float3 EvalSH9Cosine(float3 dir, SH9Color sh)
{
    SH9 dirSH = ProjectOntoSH9(dir);
    dirSH.coefficients[0] *= CosineA0;
    dirSH.coefficients[1] *= CosineA1;
    dirSH.coefficients[2] *= CosineA1;
    dirSH.coefficients[3] *= CosineA1;
    dirSH.coefficients[4] *= CosineA2;
    dirSH.coefficients[5] *= CosineA2;
    dirSH.coefficients[6] *= CosineA2;
    dirSH.coefficients[7] *= CosineA2;
    dirSH.coefficients[8] *= CosineA2;

    float3 result = 0.0;
    for (uint i = 0; i < 9; ++i)
    {
        result += dirSH.coefficients[i] * sh.coefficients[i];
    }

    return result;
}

// http://www.gamedev.net/topic/671562-spherical-harmonics-cubemap/
float3 getSphericalHarmonicDiffuse(float3 surfaceNormal)
{
    SH9Color sh;
    sh.coefficients[0] = float3(1.544518, 1.517793, 1.422065);
    sh.coefficients[1] = float3(0.815147, 0.901850, 0.955648);
    sh.coefficients[2] = float3(0.082595, 0.124363, 0.155639);
    sh.coefficients[3] = float3(-0.088787, -0.095615, -0.087551);
    sh.coefficients[4] = float3(-0.029684, -0.030612, -0.036516);
    sh.coefficients[5] = float3(0.081306, 0.076570, 0.076694);
    sh.coefficients[6] = float3(-0.183242, -0.226157, -0.270554);
    sh.coefficients[7] = float3(-0.059880, -0.032156, 0.006691);
    sh.coefficients[8] = float3(-0.112564, -0.134566, -0.186299);
    return EvalSH9Cosine(surfaceNormal, sh);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

#ifdef true
    float3 ambient = getSphericalHarmonicDiffuse(surfaceNormal);
#else
    float3 ambient = getHemisphericalAmbient(surfaceNormal);
#endif
    return ambient;// *materialAlbedo * AmbientLevel;
}

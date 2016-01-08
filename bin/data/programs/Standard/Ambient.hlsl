#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float3 SkyColor = float3((88.0f / 255.0f), (58.0f / 255.0f), (129.0f / 25.0f));
static const float3 HorizonColor = float3(0, 0, 0);
static const float3 GroundColor = float3((224.0f / 255.0f), (252.0f / 255.0f), (255.0f / 255.0f));
static const float AmbientLevel = 0.001f;

// http://www.gamedev.net/topic/673746-gradient-ambient-in-unity-how-is-it-achieved/
float3 getHemisphericalAmbient(InputPixel inputPixel)
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    float up = saturate(surfaceNormal.y);
    float down = saturate(-surfaceNormal.y);
    float middle = 1 - abs(surfaceNormal.y);

    return materialAlbedo * AmbientLevel * (GroundColor * up + SkyColor * down + HorizonColor * middle);
}

static const float Pi = 3.141592654f;
static const float CosineA0 = Pi;
static const float CosineA1 = (2.0f * Pi) / 3.0f;
static const float CosineA2 = Pi * 0.25f;

struct SH9
{
    float c[9];
};

struct SH9Color
{
    float3 c[9];
};

SH9 getSphereicalHarmonicCosineLobe(float3 direction)
{
    SH9 sh;

    // Band 0
    sh.c[0] = 0.282095f * CosineA0;

    // Band 1
    sh.c[1] = 0.488603f * direction.y * CosineA1;
    sh.c[2] = 0.488603f * direction.z * CosineA1;
    sh.c[3] = 0.488603f * direction.x * CosineA1;

    // Band 2
    sh.c[4] = 1.092548f * direction.x * direction.y * CosineA2;
    sh.c[5] = 1.092548f * direction.y * direction.z * CosineA2;
    sh.c[6] = 0.315392f * (3.0f * direction.z * direction.z - 1.0f) * CosineA2;
    sh.c[7] = 1.092548f * direction.x * direction.z * CosineA2;
    sh.c[8] = 0.546274f * (direction.x * direction.x - direction.y * direction.y) * CosineA2;

    return sh;
}

float3 getSphericalHarmonicIrradiance(float3 normal, SH9Color radiance)
{
    // Compute the cosine lobe in SH, oriented about the normal direction
    SH9 consineLobe = getSphereicalHarmonicCosineLobe(normal);

    // Compute the SH dot product to get irradiance
    float3 irradiance = 0.0f;
    for (uint index = 0; index < 9; ++index)
    {
        irradiance += radiance.c[index] * consineLobe.c[index];
    }

    return irradiance;
}

// http://www.gamedev.net/topic/671562-spherical-harmonics-cubemap/
float3 getSphericalHarmonicDiffuse(InputPixel inputPixel)
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    SH9Color radiance;
    radiance.c[0] = float3(0.078908f, 0.043710f, 0.054161f);
    radiance.c[1] = float3(0.039499f, 0.034989f, 0.060488f);
    radiance.c[2] = float3(-0.033974f, -0.018236f, -0.026940f);
    radiance.c[3] = float3(-0.029213f, -0.005562f, 0.000944f);
    radiance.c[4] = float3(-0.011141f, -0.005090f, -0.012231f);
    radiance.c[5] = float3(-0.026240f, -0.022401f, -0.047479f);
    radiance.c[6] = float3(-0.015570f, -0.009471f, -0.014733f);
    radiance.c[7] = float3(0.056014f, 0.021444f, 0.013915f);
    radiance.c[8] = float3(0.021205f, -0.005432f, -0.030374f);

    // Diffuse BRDF is albedo / Pi
    return getSphericalHarmonicIrradiance(surfaceNormal, radiance) * materialAlbedo * (1.0f / Pi);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return getSphericalHarmonicDiffuse(inputPixel);
    return getHemisphericalAmbient(inputPixel);
}

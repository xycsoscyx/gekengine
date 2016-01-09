#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float3 SkyColor = float3((88.0 / 255.0), (58.0 / 255.0), (129.0 / 25.0));
static const float3 HorizonColor = float3(0, 0, 0);
static const float3 GroundColor = float3((224.0 / 255.0), (252.0 / 255.0), (255.0 / 255.0));
static const float AmbientLevel = 0.001f;

// http://www.gamedev.net/topic/673746-gradient-ambient-in-unity-how-is-it-achieved/
float3 getHemisphericalAmbient(InputPixel inputPixel)
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    float up = saturate(surfaceNormal.y);
    float down = saturate(-surfaceNormal.y);
    float middle = 1.0 - abs(surfaceNormal.y);

    return materialAlbedo * (GroundColor * up + SkyColor * down + HorizonColor * middle);
}

static const float CosineA0 = Math::Pi;
static const float CosineA1 = (2.0 * Math::Pi) / 3.0;
static const float CosineA2 = Math::Pi * 0.25;

void getSphereicalHarmonicCosineLobe(inout float3 coefficients[9], float3 direction)
{
    // Band 0
    coefficients[0] = 0.282095 * CosineA0;

    // Band 1
    coefficients[1] = 0.488603 * direction.y * CosineA1;
    coefficients[2] = 0.488603 * direction.z * CosineA1;
    coefficients[3] = 0.488603 * direction.x * CosineA1;

    // Band 2
    coefficients[4] = 1.092548 * direction.x * direction.y * CosineA2;
    coefficients[5] = 1.092548 * direction.y * direction.z * CosineA2;
    coefficients[6] = 0.315392 * (3.0 * direction.z * direction.z - 1.0) * CosineA2;
    coefficients[7] = 1.092548 * direction.x * direction.z * CosineA2;
    coefficients[8] = 0.546274 * (direction.x * direction.x - direction.y * direction.y) * CosineA2;
}

float3 getSphericalHarmonicIrradiance(float3 normal, float3 radiance[9])
{
    // Compute the cosine lobe in SH, oriented about the normal direction
    float3 coefficients[9];
    getSphereicalHarmonicCosineLobe(coefficients, normal);

    // Compute the SH dot product to get irradiance
    float3 irradiance = 0.0;
    for (uint index = 0; index < 9; ++index)
    {
        irradiance += radiance[index] * coefficients[index];
    }

    return irradiance;
}

// http://www.gamedev.net/topic/671562-spherical-harmonics-cubemap/
float3 getSphericalHarmonicDiffuse(InputPixel inputPixel)
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    float3 coefficients[9] =
    {
        float3( 1.995419,  2.003088,  1.821823),
        float3(-1.006211, -1.039061, -1.129413),
        float3( 0.239883,  0.222330,  0.261833),
        float3( 0.029817,  0.023817,  0.030901),
        float3(-0.005715, -0.006985, -0.001721),
        float3( 0.015700,  0.011130,  0.021361),
        float3(-0.090218, -0.058563, -0.090928),
        float3( 0.037282,  0.034900,  0.024507),
        float3(-0.275658, -0.241120, -0.266649),
    };

    // Diffuse BRDF is albedo / Pi
    return getSphericalHarmonicIrradiance(surfaceNormal, coefficients) * materialAlbedo * (1.0 / Math::Pi);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
#ifdef true
    float3 ambient = getSphericalHarmonicDiffuse(inputPixel);
#else
    float3 ambient = getHemisphericalAmbient(inputPixel);
#endif
    return ambient * AmbientLevel;
}

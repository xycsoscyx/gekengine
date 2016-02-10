#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float AmbientLevel = 0.001;

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

static const float CosineA0 = Math::Pi;
static const float CosineA1 = (2.0 * Math::Pi) / 3.0;
static const float CosineA2 = Math::Pi * 0.25;

struct SH9
{
    float coefficients[9];
};

struct SH9Color
{
    float3 coefficients[9];
};

float3 EvaluateSphericalHarmonics(float3 normal, SH9Color radiance)
{
    static const float C1 = 0.429043;
    static const float C2 = 0.511664;
    static const float C3 = 0.743125;
    static const float C4 = 0.886227;
    static const float C5 = 0.247708;

    return

        // constant term, lowest frequency //////
        C4 * radiance.coefficients[0] +

        // axis aligned terms ///////////////////
        2.0 * C2 * radiance.coefficients[1] * normal.y +
        2.0 * C2 * radiance.coefficients[2] * normal.z +
        2.0 * C2 * radiance.coefficients[3] * normal.x +

        // band 2 terms /////////////////////////
        2.0 * C1 * radiance.coefficients[4] * normal.x * normal.y +
        2.0 * C1 * radiance.coefficients[5] * normal.y * normal.z +
        C3 * radiance.coefficients[6] * normal.z * normal.z - C5 * radiance.coefficients[6] +
        2.0 * C1 * radiance.coefficients[7] * normal.x * normal.z +
        C1 * radiance.coefficients[8] * (normal.x * normal.x - normal.y * normal.y);
}

// http://www.gamedev.net/topic/671562-spherical-harmonics-cubemap/
float3 getSphericalHarmonicDiffuse(float3 surfaceNormal)
{
    SH9Color radiance;
#include "radiance.h"
    return EvaluateSphericalHarmonics(surfaceNormal, radiance);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    float3 ambient = getSphericalHarmonicDiffuse(surfaceNormal);
    //float3 ambient = getHemisphericalAmbient(surfaceNormal);

    return ambient * materialAlbedo * AmbientLevel * 0.5;
}

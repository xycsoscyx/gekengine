#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

static const float3 SkyColor = float3((88.0f / 255.0f), (58.0f / 255.0f), (129.0f / 25.0f));
static const float3 HorizonColor = float3(0, 0, 0);
static const float3 GroundColor = float3((224.0f / 255.0f), (252.0f / 255.0f), (255.0f / 255.0f));
static const float ambientLevel = 0.01f;

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texCoord);
    float3 surfaceNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texCoord));
    surfaceNormal = mul(surfaceNormal, (float3x3)Camera::viewMatrix);

    // hemispherical ambient, http://www.gamedev.net/topic/673746-gradient-ambient-in-unity-how-is-it-achieved/
    float up = saturate(surfaceNormal.y);
    float down = saturate(-surfaceNormal.y);
    float middle = 1 - abs(surfaceNormal.y);
    
    return materialAlbedo * ambientLevel * (GroundColor * up + SkyColor * down + HorizonColor * middle);
}

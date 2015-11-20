#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Basic.h"

float3 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float2 materialInfo = Resources::materialBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float materialRoughness = materialInfo.x;
    float materialMetalness = materialInfo.y;

    float pixelDepth = Resources::depthBuffer.Sample(Global::pointSampler, inputPixel.texcoord);
    float3 pixelPosition = getViewPosition(inputPixel.texcoord, pixelDepth);
    float3 pixelNormal = decodeNormal(Resources::normalBuffer.Sample(Global::pointSampler, inputPixel.texcoord));

    float3 viewNormal = -normalize(pixelPosition);

    const uint2 tilePosition = uint2(floor(inputPixel.position.xy / float(lightTileSize).xx));
    const uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
    const uint bufferIndex = (tileIndex * Lighting::listSize);

    float3 surfaceColor = 0.0f;

    [loop]
    for (uint lightIndexIndex = 0; lightIndexIndex < Lighting::count; lightIndexIndex++)
    {
        uint lightIndex = Resources::tileIndexList[bufferIndex + lightIndexIndex];

        [branch]
        if (lightIndex == Lighting::listSize)
        {
            break;
        }

        float3 lightVector = (Lighting::list[lightIndex].position.xyz - pixelPosition);
        float lightDistance = length(lightVector);
        float3 lightNormal = normalize(lightVector);

        float attenuation = (lightDistance / Lighting::list[lightIndex].range);

#ifdef _INVERSE_SQUARE
        attenuation = saturate(1.0 - (attenuation * attenuation * attenuation * attenuation));
        attenuation = (attenuation * attenuation);
        attenuation = attenuation / ((lightDistance * lightDistance) + 1);
#else
        attenuation = (1.0f - saturate(attenuation));
#endif

        [branch]
        if (attenuation > 0.0f)
        {
            surfaceColor += (getBRDF(materialAlbedo, materialRoughness, materialMetalness, pixelNormal, lightNormal, viewNormal) * Lighting::list[lightIndex].color * attenuation);
        }
    }

    return surfaceColor;
}

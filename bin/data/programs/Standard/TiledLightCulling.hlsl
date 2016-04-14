#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

groupshared uint    tileMinimumDepth;
groupshared uint    tileMaximumDepth;
groupshared uint    tileLightCount;
groupshared uint    tileLightList[Lighting::maximumListSize];
groupshared float4  tileFrustum[6];

[numthreads(uint(tileSize), uint(tileSize), 1)]
void mainComputeProgram(uint3 screenPosition : SV_DispatchThreadID, uint3 tilePosition : SV_GroupID, uint pixelIndex : SV_GroupIndex)
{
    [branch]
    if (pixelIndex == 0)
    {
        tileLightCount = 0;
        tileMinimumDepth = 0x7F7FFFFF;
        tileMaximumDepth = 0;
    }

    float viewDepth = Resources::depthBuffer[screenPosition.xy] * Camera::maximumDistance;
    uint viewDepthInteger = asuint(viewDepth);

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(tileMinimumDepth, viewDepthInteger);
    InterlockedMax(tileMaximumDepth, viewDepthInteger);

    GroupMemoryBarrierWithGroupSync();

    float minimumDepth = asfloat(tileMinimumDepth);
    float maximumDepth = asfloat(tileMaximumDepth);

    [branch]
    if (pixelIndex == 0)
    {
        float2 depthBufferSize = float2(Shader::defaultWidth, Shader::defaultHeight);
        float2 tileScale = depthBufferSize * rcp(float(2 * tileSize));
        float2 tileBias = tileScale - float2(tilePosition.xy);

        float3 frustumXPlane = float3(Camera::projectionMatrix[0][0] * tileScale.x, 0.0, tileBias.x);
        float3 frustumYPlane = float3(0.0, -Camera::projectionMatrix[1][1] * tileScale.y, tileBias.y);
        float3 frustumZPlane = float3(0.0, 0.0, 1.0);

        tileFrustum[0] = float4(normalize(frustumZPlane - frustumXPlane), 0.0),
        tileFrustum[1] = float4(normalize(frustumZPlane + frustumXPlane), 0.0),
        tileFrustum[2] = float4(normalize(frustumZPlane - frustumYPlane), 0.0),
        tileFrustum[3] = float4(normalize(frustumZPlane + frustumYPlane), 0.0),
        tileFrustum[4] = float4(0.0, 0.0, 1.0, -minimumDepth);
        tileFrustum[5] = float4(0.0, 0.0, -1.0, maximumDepth);
    }

    GroupMemoryBarrierWithGroupSync();

    [loop]
    for (uint lightIndex = pixelIndex; lightIndex < Lighting::count; lightIndex += (tileSize * tileSize))
    {
        Lighting::Data light = Lighting::list[lightIndex];
        bool isLightVisible = true;

        [unroll]
        for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            float lightDistance = dot(tileFrustum[planeIndex], float4(light.transform._41_42_43, 1.0));
            isLightVisible = (isLightVisible && (lightDistance >= -light.range));
        }

        [branch]
        if (isLightVisible)
        {
            uint tileIndex;
            InterlockedAdd(tileLightCount, 1, tileIndex);
            tileLightList[tileIndex] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if (pixelIndex < Lighting::maximumListSize)
    {
        uint tileIndex = ((tilePosition.y * dispatchWidth) + tilePosition.x);
        uint bufferIndex = ((tileIndex * Lighting::maximumListSize) + pixelIndex);
        uint lightIndex = (pixelIndex < tileLightCount ? tileLightList[pixelIndex] : Lighting::maximumListSize);
        UnorderedAccess::tileIndexList[bufferIndex] = lightIndex;
    }
}

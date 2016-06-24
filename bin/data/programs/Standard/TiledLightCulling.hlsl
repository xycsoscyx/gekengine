#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

namespace Defines
{
    static const uint   tileVolume = (Defines::tileSize * Defines::tileSize);
};

namespace Shared
{
    groupshared uint    tileMinimumDepth;
    groupshared uint    tileMaximumDepth;
    groupshared uint    tileLightCount;
    groupshared uint    tileLightList[Lighting::lightsPerPass];
    groupshared float4  tileFrustum[6];
};

namespace Light
{
    bool isPointVisible(Lighting::Data light)
    {
        float4 position = float4(light.position, 1.0);
        bool isLightVisible = true;

        [unroll]
        for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            float lightDistance = dot(Shared::tileFrustum[planeIndex], position);
            isLightVisible = (isLightVisible && (lightDistance >= -light.range));
        }

        return isLightVisible;
    }

    bool isDirectionalVisible(Lighting::Data light)
    {
        return true;
    }

    bool isSpotVisible(Lighting::Data light)
    {
        float halfRange = (light.range * 0.5);
        float4 center = float4(light.position + (light.direction * halfRange), 1.0);
        bool isLightVisible = true;

        [unroll]
        for (uint planeIndex = 0; planeIndex < 6; ++planeIndex)
        {
            float lightDistance = dot(Shared::tileFrustum[planeIndex], center);
            isLightVisible = (isLightVisible && (lightDistance >= -halfRange));
        }

        return isLightVisible;
    }

    bool isVisible(Lighting::Data light)
    {
        [branch]
        switch (light.type)
        {
        case Lighting::Type::Point:
            return isPointVisible(light);

        case Lighting::Type::Directional:
            return isDirectionalVisible(light);

        case Lighting::Type::Spot:
            return isSpotVisible(light);
        };

        return false;
    }
};

[numthreads(uint(Defines::tileSize), uint(Defines::tileSize), 1)]
void mainComputeProgram(uint3 screenPosition : SV_DispatchThreadID, uint3 tilePosition : SV_GroupID, uint pixelIndex : SV_GroupIndex)
{
    [branch]
    if (pixelIndex == 0)
    {
        Shared::tileLightCount = 0;
        Shared::tileMinimumDepth = 0x7F7FFFFF;
        Shared::tileMaximumDepth = 0;
    }

    float viewDepth = getLinearDepth(Resources::depth[screenPosition.xy]);
    uint viewDepthInteger = asuint(viewDepth);

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin(Shared::tileMinimumDepth, viewDepthInteger);
    InterlockedMax(Shared::tileMaximumDepth, viewDepthInteger);

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if (pixelIndex == 0)
    {
        float2 depthBufferSize = Shader::targetSize;
        float2 tileScale = (depthBufferSize * rcp(float(2.0 * Defines::tileSize)));
        float2 tileBias = tileScale - float2(tilePosition.xy);

        float3 frustumXPlane = float3(Camera::projectionMatrix[0][0] * tileScale.x, 0.0, tileBias.x);
        float3 frustumYPlane = float3(0.0, -Camera::projectionMatrix[1][1] * tileScale.y, tileBias.y);
        float3 frustumZPlane = float3(0.0, 0.0, 1.0);

        Shared::tileFrustum[0] = float4(normalize(frustumZPlane - frustumXPlane), 0.0),
        Shared::tileFrustum[1] = float4(normalize(frustumZPlane + frustumXPlane), 0.0),
        Shared::tileFrustum[2] = float4(normalize(frustumZPlane - frustumYPlane), 0.0),
        Shared::tileFrustum[3] = float4(normalize(frustumZPlane + frustumYPlane), 0.0),
        Shared::tileFrustum[4] = float4(0.0, 0.0, 1.0, -asfloat(Shared::tileMinimumDepth));
        Shared::tileFrustum[5] = float4(0.0, 0.0, -1.0, asfloat(Shared::tileMaximumDepth));
    }

    GroupMemoryBarrierWithGroupSync();

    [loop]
    for (uint lightIndex = pixelIndex; lightIndex < Lighting::count; lightIndex += Defines::tileVolume)
    {
        Lighting::Data light = Lighting::list[lightIndex];

        [branch]
        if (Light::isVisible(light))
        {
            uint tileIndex;
            InterlockedAdd(Shared::tileLightCount, 1, tileIndex);
            Shared::tileLightList[tileIndex] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if (pixelIndex < Lighting::lightsPerPass)
    {
        uint tileIndex = ((tilePosition.y * Defines::dispatchWidth) + tilePosition.x);
        uint bufferIndex = ((tileIndex * (Lighting::lightsPerPass + 1)) + pixelIndex);

        [branch]
        if (pixelIndex == 0)
        {
            UnorderedAccess::tileIndexList[bufferIndex] = Shared::tileLightCount;
        }

        UnorderedAccess::tileIndexList[bufferIndex + 1] = Shared::tileLightList[pixelIndex];
    }
}

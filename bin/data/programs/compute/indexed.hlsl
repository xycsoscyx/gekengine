#define TILE_LIGHT_SIZE                 32
#define THREAD_GROUP_SIZE               (TILE_LIGHT_SIZE * TILE_LIGHT_SIZE)
#define MAX_LIGHTS                      255

cbuffer ENGINEBUFFER                    : register(b0)
{
    float4x4 gs_nViewMatrix;
    float4x4 gs_nProjectionMatrix;
    float4x4 gs_nTransformMatrix;
    float3   gs_nCameraPosition;
    float    gs_nCameraViewDistance;
    float2   gs_nCameraView;
    float2   gs_nPadding;
};

struct LIGHT
{
    float4 m_nPosition;
    float3 m_nColor;
    float  m_nInvRange;
};

Texture2D     gs_pWorldBuffer           : register(t1);
StructuredBuffer<LIGHT> gs_aLights      : register(t2);

RWBuffer<uint> gs_pTileOutput           : register(u0);

// Shared memory
groupshared uint g_nTileMinDepth;
groupshared uint g_nTileMaxDepth;

// Light list for the tile
groupshared uint g_aTileLightList[MAX_LIGHTS];
groupshared uint g_nNumTilesLights;

float GetLinearFromView(in float nViewDepth)
{
    return gs_nProjectionMatrix._43 / (nViewDepth - gs_nProjectionMatrix._33);
}

[numthreads(TILE_LIGHT_SIZE, TILE_LIGHT_SIZE, 1)]
void MainComputeProgram(uint3 nGroupID : SV_GroupID, uint3 nGroupThreadID : SV_GroupThreadID)
{
	uint2 nPixelCoord = nGroupID.xy * uint2(TILE_LIGHT_SIZE, TILE_LIGHT_SIZE) + nGroupThreadID.xy;

	const uint nGroupThreadIndex = nGroupThreadID.y * TILE_LIGHT_SIZE + nGroupThreadID.x;

	// Work out Z bounds for our samples
    float nMinSampleZ = gs_nCameraViewDistance;
    float nMaxSampleZ = 0.0f;

    // Calculate view-space Z from Z/W depth
    float4 nWorld = gs_pWorldBuffer[nPixelCoord];
    float nViewDepth = nWorld.w;

	float nLinearDepth = GetLinearFromView(nViewDepth);
    nMinSampleZ = min(nMinSampleZ, nLinearDepth);
    nMaxSampleZ = max(nMaxSampleZ, nLinearDepth);

    // Initialize shared memory light list and Z bounds
    if(nGroupThreadIndex == 0)
	{
        g_nNumTilesLights = 0;
        g_nTileMinDepth = 0x7F7FFFFF;      // Max float
        g_nTileMaxDepth = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if(nMaxSampleZ >= nMinSampleZ)
	{
        InterlockedMin(g_nTileMinDepth, asuint(nMinSampleZ));
        InterlockedMax(g_nTileMaxDepth, asuint(nMaxSampleZ));
    }

    GroupMemoryBarrierWithGroupSync();

    float nTileMinDepth = asfloat(g_nTileMinDepth);
    float nTileMaxDepth = asfloat(g_nTileMaxDepth);

    // Work out scale/bias from [0, 1]
    float2 nTileScale = float2(1280, 800) * rcp(2.0f * float2(TILE_LIGHT_SIZE, TILE_LIGHT_SIZE));
    float2 nTileBias = nTileScale - float2(nGroupID.xy);

    // Now work out composite projection matrix
    // Relevant matrix columns for this tile frusta
    float4 c1 = float4(gs_nProjectionMatrix._11 * nTileScale.x, 0.0f, nTileBias.x, 0.0f);
    float4 c2 = float4(0.0f, -gs_nProjectionMatrix._22 * nTileScale.y, nTileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Derive frustum planes
    float4 aFrustumPlanes[6] = 
    {
        // Sides
        c4 - c1,
        c4 + c1,
        c4 - c2,
        c4 + c2,

        // Near/far
        float4(0.0f, 0.0f, 1.0f, -nTileMinDepth),
        float4(0.0f, 0.0f, -1.0f, nTileMaxDepth),
    };

    // Normalize frustum planes (near/far already normalized)
    [unroll]
	for (uint nIndex = 0; nIndex < 4; ++nIndex)
    {
        aFrustumPlanes[nIndex] *= rcp(length(aFrustumPlanes[nIndex].xyz));
    }

    // Cull lights for this tile
    [unroll]
    for (uint nLightIndex = nGroupThreadIndex; nLightIndex < MAX_LIGHTS; nLightIndex += THREAD_GROUP_SIZE)
	{
        float3 nLightPosition = gs_aLights[nLightIndex].m_nPosition.xyz;
        float nLightRange = 1.0f / gs_aLights[nLightIndex].m_nInvRange;

        // Cull: point light sphere vs tile frustum
        bool bIsInFrustum = true;

        [unroll]
		for(uint nIndex = 0; nIndex < 6; ++nIndex)
		{
            float nDistance = dot(aFrustumPlanes[nIndex], float4(nLightPosition, 1.0f));
            bIsInFrustum = bIsInFrustum && (nDistance >= -nLightRange);
        }

        [branch]
		if(bIsInFrustum)
		{
            uint nListIndex;
            InterlockedAdd(g_nNumTilesLights, 1, nListIndex);
            g_aTileLightList[nListIndex] = nLightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if(nGroupThreadIndex < MAX_LIGHTS)
    {
        // Write out the indices
        uint nTileIndex = nGroupID.y * NumTiles.x + nGroupID.x;
        uint nBufferIndex = nTileIndex * MAX_LIGHTS + nGroupThreadIndex;
        uint nLightIndex = nGroupThreadIndex < g_nNumTilesLights ? g_aTileLightList[nGroupThreadIndex] : MAX_LIGHTS;
        gs_pTileOutput[nBufferIndex] = nLightIndex;
    }
}
#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

[numthreads(uint(tileSize), uint(tileSize), 1)]
void mainComputeProgram(uint3 screenPosition : SV_DispatchThreadID, uint3 tilePosition : SV_GroupID, uint pixelIndex : SV_GroupIndex)
{
    [branch]
    if (pixelIndex == 0)
    {
    }

    float3 color = Resources::compositeBuffer[screenPosition.xy];

    GroupMemoryBarrierWithGroupSync();

    [branch]
    if (pixelIndex == 0)
    {
        UnorderedAccess::exposureBuffer[0] = 10.0f;
    }
}

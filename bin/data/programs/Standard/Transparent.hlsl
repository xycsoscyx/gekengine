#include "GEKGlobal.h"
#include "GEKtypes.h"

Texture2D     gs_pAlbedoMap             : register(t0);

float4 MainPixelProgram(in INPUT kInput) : SV_TARGET0
{
    return (gs_pAlbedoMap.Sample(gs_pLinearSampler, kInput.texcoord) * kInput.color);
}

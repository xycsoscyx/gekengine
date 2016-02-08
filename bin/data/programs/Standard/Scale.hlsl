#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return Resources::albedo.Sample(Global::linearSampler, inputPixel.texCoord);

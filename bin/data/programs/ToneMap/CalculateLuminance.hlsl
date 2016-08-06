#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return log(getLuminance(Resources::finalBuffer[inputPixel.screen.xy]));
}

#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
    return log(getLuminance(Resources::screen[inputPixel.screen.xy]));
}

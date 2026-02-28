#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const uint2 screenCoord = uint2(inputPixel.screen.xy);
    return log(GetLuminance(Resources::inputBuffer[screenCoord].xyz));
}

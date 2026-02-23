#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    return log(GetLuminance(Resources::inputBuffer[inputPixel.screen.xy].xyz));
}

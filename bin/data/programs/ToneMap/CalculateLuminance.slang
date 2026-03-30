#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>

[shader("fragment")]
float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    uint textureWidth = 0;
    uint textureHeight = 0;
    Resources::inputBuffer.GetDimensions(textureWidth, textureHeight);
    const int2 maxCoord = int2(int(max(textureWidth, 1u)) - 1, int(max(textureHeight, 1u)) - 1);
    const uint2 screenCoord = uint2(clamp(int2(inputPixel.screen.xy), int2(0, 0), maxCoord));
    return log(GetLuminance(Resources::inputBuffer[screenCoord].xyz));
}

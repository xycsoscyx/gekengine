#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float depth = 
        Resources::depthBufferMS.Load(inputPixel.screen.xy, 0) +
        Resources::depthBufferMS.Load(inputPixel.screen.xy, 1) +
        Resources::depthBufferMS.Load(inputPixel.screen.xy, 2) +
        Resources::depthBufferMS.Load(inputPixel.screen.xy, 3);
    return depth * 0.25;
}

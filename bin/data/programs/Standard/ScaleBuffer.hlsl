#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

#include "BRDF.Custom.h"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    OutputPixel output;
    output.destination = Resources::source.Sample(Global::linearClampSampler, inputPixel.texCoord);
    return output;
}

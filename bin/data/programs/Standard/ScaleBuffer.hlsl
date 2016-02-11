#include "GEKEngine"

#include "GEKGlobal.h"
#include "GEKUtility.h"

#include "BRDF.Custom.h"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    OutputPixel output;
    output.destination = Resources::source.Sample(Global::linearSampler, inputPixel.texCoord);
    return output;
}

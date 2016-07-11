#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float3 pixelColor = Resources::finalBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);

    OutputPixel output;
    output.luminanceBuffer = log(getLuminance(pixelColor));
    return output;
}

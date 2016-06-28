#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

OutputPixel mainPixelProgram(InputPixel inputPixel)
{
    float3 pixelColor = Resources::finalBuffer.Sample(Global::pointSampler, inputPixel.texCoord);

    OutputPixel output;
    output.luminanceBuffer = log(getLuminance(pixelColor));
    return output;
}

#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float getBlurredDepth(in float2 texCoord) // blurring depth
{
    static const float2 blurOffset = (Shader::pixelSize * Defines::depthBlurSize);
    static const float2 offset[9] =
    {
        float2(-blurOffset.x, -blurOffset.y), float2(0.0, -blurOffset.y), float2(blurOffset.x, -blurOffset.y), 
        float2(-blurOffset.x,           0.0), float2(0.0,           0.0), float2(blurOffset.x,           0.0), 
        float2(-blurOffset.x,  blurOffset.y), float2(0.0,  blurOffset.y), float2(blurOffset.x,  blurOffset.y), 
    };

    static const float kernel[9] = 
    {
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
        (2.0 / 16.0), (4.0 / 16.0), (2.0 / 16.0),
        (1.0 / 16.0), (2.0 / 16.0), (1.0 / 16.0),
    };

    float blurredDepth = 0.0;

    [unroll]
    for (int index = 0; index < 9; index++)
    {
        blurredDepth += (getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord + offset[index], 0)) * kernel[index]);
    }

    return blurredDepth;
}

float getDepth(im float2 texCoord)
{
    if (Defines::enableDepthBlur)
    {
        return getBlurredDepth(texCoord);
    }
    else
    {
        return getLinearDepthFromSample(Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord, 0));
    }
}

OutputPixel mainPixelProgram(im InputPixel inputPixel)
{
	static const float reciprocalFocalRange = (1.0 / Defines::focalRange);

    const float3 screenColor = Resources::screen[inputPixel.screen.xy];
    const float focalDepth = Resources::averageFocalDepth[0];
    const float sceneDepth = getDepth(inputPixel.texCoord);

    OutputPixel outputPixel;
	outputPixel.circleOfConfusion = clamp(((sceneDepth - focalDepth) * reciprocalFocalRange), -1.0, 1.0);
    //outputPixel.foregroundBuffer = (screenColor * saturate(-outputPixel.circleOfConfusion));
	outputPixel.screenBuffer = screenColor;
    return outputPixel;
}
#include "GEKEngine"

#include "GEKGlobal.hlsl"

namespace Defines
{
    static const float adaptionRate = 1.25;
};

OutputPixel mainPixelProgram(void)
{
    float averageLuminance = Resources::averageLuminanceBuffer.Load(uint3(0, 0, 0));
    float currentLuminance = Resources::luminanceBuffer.Load(uint3(0, 0, 9));
    averageLuminance += (currentLuminance - averageLuminance) * (1.0 - exp(-Engine::frameTime * Defines::adaptionRate));

    OutputPixel outputPixel;
    outputPixel.averageLuminanceBuffer = averageLuminance;
    return outputPixel;
}

#include "GEKEngine"

#include "GEKGlobal.hlsl"

float mainPixelProgram(void) : SV_TARGET0
{
    float averageLuminance = Resources::averageLuminanceBuffer.Load(uint3(0, 0, 0));
    float currentLuminance = exp2(Resources::luminanceBuffer.Load(uint3(0, 0, 10)));
    return currentLuminance;
    return averageLuminance + (currentLuminance - averageLuminance) * (1.0 - exp(-(1.0 / 240.0) * Math::Tau));
}

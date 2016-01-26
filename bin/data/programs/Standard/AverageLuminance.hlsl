#include "GEKEngine"

#include "GEKGlobal.h"

float mainPixelProgram(void) : SV_TARGET0
{
    float previousluminance = Resources::previousLuminance.Load(uint3(0, 0, 0));
    float currentLuminance = exp2(Resources::luminanceBuffer.Load(uint3(1, 1, 8)));
    return previousluminance + (currentLuminance - previousluminance) * (1.0 - exp(-(1.0 / 120.0) * Math::Tau));
}

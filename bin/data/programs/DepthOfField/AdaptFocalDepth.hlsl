#include "GEKFilter"

#include "GEKGlobal.hlsl"

namespace Defines
{
    static const float adaptionRate = 2.0;
};

[numthreads(uint(1), uint(1), 1)]
void mainComputeProgram(void)
{
    float averageDepth = UnorderedAccess::focalDepth[0];
    float currentDepth = Resources::depth.Load(int3((Shader::targetSize *  0.5), 0));
    averageDepth += (currentDepth - averageDepth) * (1.0 - exp(-Engine::frameTime * Defines::adaptionRate));

    UnorderedAccess::focalDepth[0] = averageDepth;
}

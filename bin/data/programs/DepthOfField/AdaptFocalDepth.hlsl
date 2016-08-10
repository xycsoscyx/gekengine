#include "GEKFilter"

#include "GEKGlobal.hlsl"

namespace Defines
{
    static const float adaptionRate = 2.0;
};

[numthreads(uint(1), uint(1), 1)]
void mainComputeProgram(void)
{
    float averageDepth = UnorderedAccess::averageFocalDepth[0];
    float currentDepth = getLinearDepthFromSample(Resources::depthBuffer.Load(int3((Shader::targetSize *  0.5), 0)));
    averageDepth += (currentDepth - averageDepth) * (1.0 - exp(-Engine::frameTime * Defines::adaptionRate));

    UnorderedAccess::averageFocalDepth[0] = averageDepth;
}

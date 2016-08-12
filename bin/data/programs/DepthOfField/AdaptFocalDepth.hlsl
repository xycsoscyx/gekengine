#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

[numthreads(1, 1, 1)]
void mainComputeProgram(void)
{
    float averageDepth = UnorderedAccess::averageFocalDepth[0];
    float currentDepth = getLinearDepthFromSample(Resources::depthBuffer[int2(Shader::targetSize * 0.5)]);
    averageDepth += (currentDepth - averageDepth) * (1.0 - exp(-Engine::frameTime * Defines::adaptionRate));

    UnorderedAccess::averageFocalDepth[0] = averageDepth;
}

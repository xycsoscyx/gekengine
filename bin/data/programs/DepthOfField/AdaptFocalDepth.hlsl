#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

[numthreads(1, 1, 1)]
void mainComputeProgram(void)
{
    float averageDepth = UnorderedAccess::averageFocalDepth.Load(0);
    const float currentDepth = getLinearDepthFromSample(Resources::depthBuffer.Load(int3(Shader::targetSize * 0.5, 0)));
    averageDepth += (currentDepth - averageDepth) * (1.0 - exp(-Engine::frameTime * Defines::adaptionRate));
    UnorderedAccess::averageFocalDepth[0] = averageDepth;
}

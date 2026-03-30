#include <GEKEngine>

#include <GEKGlobal.slang>

[numthreads(1, 1, 1)]
[shader("compute")]
void mainComputeProgram(void)
{
    uint width, height, mipMapCount;
    Resources::luminanceBuffer.GetDimensions(0, width, height, mipMapCount);

    const uint sampledMipLevel = (mipMapCount > 0 ? mipMapCount - 1 : 0);

    float averageLuminance = UnorderedAccess::averageLuminanceBuffer[0];
    float loggedLuminance = Resources::luminanceBuffer.Load(uint3(0, 0, sampledMipLevel));
    loggedLuminance = (isfinite(loggedLuminance) ? loggedLuminance : 0.0);
    const float currentLuminance = exp(loggedLuminance);
    averageLuminance += (currentLuminance - averageLuminance) * (1.0 - exp(-Engine::FrameTime * 1.25));// Options::AdaptionRate));
    averageLuminance = (isfinite(averageLuminance) ? averageLuminance : 0.0);
    UnorderedAccess::averageLuminanceBuffer[0] = max(averageLuminance, Math::Epsilon);
}

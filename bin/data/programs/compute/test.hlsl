RWTexture2D<float4> gs_pOutput              : register(u0);

[numthreads(32, 32, 1)]
void MainComputeProgram(uint3 nThreadID : SV_DispatchThreadID)
{
    gs_pOutput[nThreadID.xy] = float4(nThreadID.xy / 1024.0f, 0, 1);
}
#include "gekengine.h"

Texture2D           gs_pAlbedoBuffer        : register(t1);
Texture2D<half2>    gs_pNormalBuffer        : register(t2);
Texture2D<float>    gs_pDepthBuffer         : register(t3);

float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    float4 nAlbedo = gs_pAlbedoBuffer.Sample(gs_pPointSampler, kInput.texcoord);
	float nCenterDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, kInput.texcoord);
    float nScaleDepth = (1 - saturate(nCenterDepth * 1.5));

    float2 nSize;
    gs_pDepthBuffer.GetDimensions(nSize.x, nSize.y);
    nSize = rcp(nSize);
                
    float2 nOffset = (gs_nRadius * nSize * nScaleDepth);
    nOffset = max(nOffset, nSize);

    float3 nSampleNormalP0 = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x, 0.0f))));
    float3 nSampleNormalN0 = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x, 0.0f))));
    float3 nSampleNormal0P = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( 0.0f, nOffset.y))));
    float3 nSampleNormal0N = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( 0.0f,-nOffset.y))));
    float3 nSampleNormalNN = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x,-nOffset.y))));
    float3 nSampleNormalPP = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x, nOffset.y))));
    float3 nSampleNormalNP = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x, nOffset.y))));
    float3 nSampleNormalPN = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x,-nOffset.y))));

    float4 nEdgeNormal;
    nEdgeNormal.x = dot(nSampleNormalP0, nSampleNormalN0);
    nEdgeNormal.y = dot(nSampleNormal0P, nSampleNormal0N);
    nEdgeNormal.z = dot(nSampleNormalNN, nSampleNormalPP);
    nEdgeNormal.w = dot(nSampleNormalNP, nSampleNormalPN);
    nEdgeNormal -= 0.7f;
    nEdgeNormal = step(0, nEdgeNormal);
    float nAverageEdgeNormal = saturate(dot(nEdgeNormal, 0.3f));

    float nSampleDepthP0 = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x, 0.0f)));
    float nSampleDepthN0 = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x, 0.0f)));
    float nSampleDepth0P = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( 0.0f, nOffset.y)));
    float nSampleDepth0N = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( 0.0f,-nOffset.y)));
    float nSampleDepthNN = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x,-nOffset.y)));
    float nSampleDepthPP = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x, nOffset.y)));
    float nSampleDepthNP = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2(-nOffset.x, nOffset.y)));
    float nSampleDepthPN = gs_pDepthBuffer.Sample(gs_pPointSampler, (kInput.texcoord + float2( nOffset.x,-nOffset.y)));
 
    float4 nEdgeDepth;
    nEdgeDepth.x = (nSampleDepthP0 + nSampleDepthN0);
    nEdgeDepth.y = (nSampleDepth0P + nSampleDepth0N);
    nEdgeDepth.z = (nSampleDepthNN + nSampleDepthPP);
    nEdgeDepth.w = (nSampleDepthNP + nSampleDepthPN);
    nEdgeDepth = (abs((2.0f * nCenterDepth) - nEdgeDepth) - 0.004f);
    nEdgeDepth = step(nEdgeDepth, 0);
    float nAverageEdgeDepth = saturate(dot(nEdgeDepth, 0.4f));

    return float4((nAlbedo.xyz * nAverageEdgeDepth * nAverageEdgeNormal), 1.0f);
}

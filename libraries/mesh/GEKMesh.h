#pragma once

#include <Windows.h>
#include "GEKUtility.h"
#include <vector>

struct VERTEX
{
    float3 position;
    float2 texcoord;
    float3 tangent;
    float3 bitangent;
    float3 normal;
};

HRESULT GEKOptimizeMesh(const VERTEX *pInputVertices, UINT32 nNumVertices, const UINT16 *pInputIndices, UINT32 nNumIndices, 
                        std::vector<VERTEX> &aOutputVertices, std::vector<UINT16> &aOutputIndices,
                        float nFaceEpsilon = 0.021f,
                        float nPartialEdgeThreshold = 0.01f,
                        float nSingularPointThreshold = 0.25f,
                        float nNormalEdgeThreshold = 0.01f);

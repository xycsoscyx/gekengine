#include "GEKMath.h"
#include "GEKUtility.h"
#include "GEKMesh.h"
#include <atlpath.h>
#include <algorithm>
#include <map>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp.lib")

struct MATERIAL
{
    UINT32 m_nFirstVertex;
    UINT32 m_nFirstIndex;
    UINT32 m_nNumIndices;
};

class CMyException
{
public:
    CStringW m_strMessage;
    int m_nLine;

public:
    CMyException(int nLine, LPCWSTR pFormat, ...)
        : m_nLine(nLine)
    {
        va_list pArgs;
        va_start(pArgs, pFormat);
        m_strMessage.FormatV(pFormat, pArgs);
        va_end(pArgs);
    }
};

void GetMeshes(const aiScene *pScene, const aiNode *pNode, const float4x4 &nParentTransform, std::map<CStringA, std::vector<VERTEX>> &aMaterials, aabb &nAABB)
{
    if (pNode == nullptr)
    {
        throw CMyException(__LINE__, L"Invalid node encountered");
    }

    float4x4 nLocalTransform = *(float4x4 *)&pNode->mTransformation;
    nLocalTransform.Transpose();

    float4x4 nTransform = (nLocalTransform * nParentTransform);
    if (pNode->mNumMeshes > 0)
    {
        if (pNode->mMeshes == nullptr)
        {
            throw CMyException(__LINE__, L"Invalid node mesh data");
        }

        for (UINT32 nMesh = 0; nMesh < pNode->mNumMeshes; nMesh++)
        {
            UINT32 nMeshIndex = pNode->mMeshes[nMesh];
            if (nMeshIndex >= pScene->mNumMeshes)
            {
                throw CMyException(__LINE__, L"Invalid mesh index encountered: %d (of %d)", nMeshIndex, pScene->mNumMeshes);
            }

            const aiMesh *pMesh = pScene->mMeshes[nMeshIndex];
            if (pMesh->mNumFaces > 0)
            {
                if (pMesh->mFaces == nullptr)
                {
                    throw CMyException(__LINE__, L"Invalid mesh face data");
                }

                if (pMesh->mVertices == nullptr)
                {
                    throw CMyException(__LINE__, L"Invalid mesh vertex data");
                }

                CStringA strMaterial;
                if (pScene->mMaterials)
                {
                    const aiMaterial *pMaterial = pScene->mMaterials[pMesh->mMaterialIndex];
                    if (pMaterial)
                    {
                        aiString strPath;
                        pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &strPath);
                        
                        CPathA kPath = strPath.C_Str();
                        kPath.RemoveExtension();
                        strMaterial = kPath.m_strPath;
                        int nTexturesRoot = strMaterial.Find("/textures/");
                        if (nTexturesRoot >= 0)
                        {
                            strMaterial = strMaterial.Mid(nTexturesRoot + 10);
                        }
                    }
                }

                std::vector<VERTEX> &aVertices = aMaterials[strMaterial];
                for (UINT32 nFace = 0; nFace < pMesh->mNumFaces; nFace++)
                {
                    const aiFace &kFace = pMesh->mFaces[nFace];
                    for (UINT32 nTriangle = 0; nTriangle < (kFace.mNumIndices - 2); nTriangle++)
                    {
                        for (UINT32 nEdge = 0; nEdge < 3; nEdge++)
                        {
                            UINT32 nIndex = kFace.mIndices[nTriangle + nEdge];

                            VERTEX kVertex;
                            kVertex.position.x = pMesh->mVertices[nIndex].x;
                            kVertex.position.y = pMesh->mVertices[nIndex].y;
                            kVertex.position.z = pMesh->mVertices[nIndex].z;
                            kVertex.position = (nTransform * float4(kVertex.position, 1.0f));
                            nAABB.Extend(kVertex.position);

                            if (pMesh->mTextureCoords)
                            {
                                kVertex.texcoord.x = pMesh->mTextureCoords[0][nIndex].x;
                                kVertex.texcoord.y = pMesh->mTextureCoords[0][nIndex].y;
                            }

                            if (pMesh->mTangents)
                            {
                                kVertex.tangent.x = pMesh->mTangents[nIndex].x;
                                kVertex.tangent.y = pMesh->mTangents[nIndex].y;
                                kVertex.tangent.z = pMesh->mTangents[nIndex].z;
                            }

                            if (pMesh->mBitangents)
                            {
                                kVertex.bitangent.x = pMesh->mBitangents[nIndex].x;
                                kVertex.bitangent.y = pMesh->mBitangents[nIndex].y;
                                kVertex.bitangent.z = pMesh->mBitangents[nIndex].z;
                            }

                            if (pMesh->mNormals)
                            {
                                kVertex.normal.x = pMesh->mNormals[nIndex].x;
                                kVertex.normal.y = pMesh->mNormals[nIndex].y;
                                kVertex.normal.z = pMesh->mNormals[nIndex].z;
                            }

                            aVertices.push_back(kVertex);
                        }
                    }
                }
            }
        }
    }

    if (pNode->mNumChildren > 0)
    {
        if (pNode->mChildren == nullptr)
        {
            throw CMyException(__LINE__, L"Invalid node children data");
        }

        for (UINT32 nChild = 0; nChild < pNode->mNumChildren; nChild++)
        {
            GetMeshes(pScene, pNode->mChildren[nChild], nTransform, aMaterials, nAABB);
        }
    }
}

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("GEK Mesh Optimizer\r\n");

    CStringW strInput;
    CStringW strOutput;
    float nFaceEpsilon = 0.025f;
    float nPartialEdgeThreshold = 0.01f;
    float nSingularPointThreshold = 0.25f;
    float nNormalEdgeThreshold = 0.01f;
    for (int nArgument = 1; nArgument < nNumArguments; nArgument++)
    {
        CStringW strArgument = astrArguments[nArgument];
        if (strArgument.CompareNoCase(L"-input") == 0 && nArgument < (nNumArguments - 1))
        {
            strInput = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-output") == 0 && nArgument < (nNumArguments - 1))
        {
            strOutput = astrArguments[++nArgument];
        }
        else if (strArgument.CompareNoCase(L"-epsilon") == 0 && nArgument < (nNumArguments - 1))
        {
            nFaceEpsilon = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-edge") == 0 && nArgument < (nNumArguments - 1))
        {
            nPartialEdgeThreshold = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-point") == 0 && nArgument < (nNumArguments - 1))
        {
            nSingularPointThreshold = StrToFloat(astrArguments[++nArgument]);
        }
        else if (strArgument.CompareNoCase(L"-normal") == 0 && nArgument < (nNumArguments - 1))
        {
            nNormalEdgeThreshold = StrToFloat(astrArguments[++nArgument]);
        }
    }

    if (strInput.IsEmpty() || strOutput.IsEmpty())
    {
        printf("Arguments:\r\n");
        printf("-input <input filename>\r\n");
        printf("-output <output filename>\r\n");
        printf("-epsilon <face epsilon>\r\n");
        printf("-edge <partial edge threshold>\r\n");
        printf("-point <singular point threshold>\r\n");
        printf("-normal <normal edge threshold>\r\n");
        return -1;
    }
    else
    {
        printf("Input: %S\r\n", strInput.GetString());
        printf("Output: %S\r\n", strOutput.GetString());
        printf("Face Epsilon: %f\r\n", nFaceEpsilon);
        printf("Partial Edge Threshold: %f\r\n", nPartialEdgeThreshold);
        printf("Singular Point Threshold: %f\r\n", nSingularPointThreshold);
        printf("Normal Edge Threshold: %f\r\n", nNormalEdgeThreshold);
    }

    try
    {
        const aiScene *pScene = aiImportFile(CW2A(strInput, CP_UTF8), aiProcessPreset_TargetRealtime_Quality | aiProcess_TransformUVCoords);
        if (pScene == nullptr)
        {
            throw CMyException(__LINE__, L"Unable to Load Input: %s", strInput.GetString());
        }

        if (pScene->mMeshes == nullptr)
        {
            throw CMyException(__LINE__, L"No meshes found in scene: %s", strInput.GetString());
        }

        aabb nAABB;
        std::map<CStringA, std::vector<VERTEX>> aMaterials;
        GetMeshes(pScene, pScene->mRootNode, float4x4(), aMaterials, nAABB);

        printf("< Num. Simplified Materials: %d\r\n", aMaterials.size());

        FILE *pFile = nullptr;
        _wfopen_s(&pFile, strOutput, L"wb");
        if (pFile != nullptr)
        {
            UINT32 nGEKX = *(UINT32 *)"GEKX";
            UINT16 nType = 0;
            UINT16 nVersion = 2;
            UINT32 nNumMaterials = aMaterials.size();
            fwrite(&nGEKX, sizeof(UINT32), 1, pFile);
            fwrite(&nType, sizeof(UINT16), 1, pFile);
            fwrite(&nVersion, sizeof(UINT16), 1, pFile);
            fwrite(&nAABB, sizeof(aabb), 1, pFile);
            fwrite(&nNumMaterials, sizeof(UINT32), 1, pFile);

            std::vector<UINT16> aTotalIndices;
            std::vector<VERTEX> aTotalVertices;
            for (auto &kPair : aMaterials)
            {
                CStringA strMaterial = (kPair.first);
                fwrite(strMaterial.GetBuffer(), (strMaterial.GetLength() + 1), 1, pFile);

                printf("-< Material: %s\r\n", strMaterial.GetString());

                std::vector<VERTEX> &aVertices = (kPair.second);
                printf("--< Num. Base Faces: %d\r\n", aVertices.size());
                printf("--< Optimizing and Generating Tangent Frame\r\n");

                std::vector<VERTEX> aOutputVertices;
                std::vector<UINT16> aOutputIndices;
                if (FAILED(GEKOptimizeMesh(&aVertices[0], aVertices.size(), nullptr, 0, aOutputVertices, aOutputIndices, true, nFaceEpsilon, nPartialEdgeThreshold, nSingularPointThreshold, nNormalEdgeThreshold)))
                {
                    throw CMyException(__LINE__, L"Unable to optimize mesh data");
                }

                UINT32 nFirstVertex = aTotalVertices.size();
                UINT32 nFirstIndex = aTotalIndices.size();
                UINT32 nNumIndices = aOutputIndices.size();
                fwrite(&nFirstVertex, sizeof(UINT32), 1, pFile);
                fwrite(&nFirstIndex, sizeof(UINT32), 1, pFile);
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);

                aTotalVertices.insert(aTotalVertices.end(), aOutputVertices.begin(), aOutputVertices.end());
                aTotalIndices.insert(aTotalIndices.end(), aOutputIndices.begin(), aOutputIndices.end());
            }

            UINT32 nNumVertices = aTotalVertices.size();
            fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
            printf("-< Num. Total Vertices: %d\r\n", aTotalVertices.size());
            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].position, sizeof(float3), 1, pFile);
            }

            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].texcoord, sizeof(float2), 1, pFile);
            }

            for (UINT32 nVertex = 0; nVertex < nNumVertices; nVertex++)
            {
                fwrite(&aTotalVertices[nVertex].tangent, sizeof(float3), 1, pFile);
                fwrite(&aTotalVertices[nVertex].bitangent, sizeof(float3), 1, pFile);
                fwrite(&aTotalVertices[nVertex].normal, sizeof(float3), 1, pFile);
            }

            UINT32 nNumIndices = aTotalIndices.size();
            fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
            fwrite(&aTotalIndices[0], sizeof(UINT16), aTotalIndices.size(), pFile);
            printf("-< Num. Total Indices: %d\r\n", aTotalIndices.size());

            fclose(pFile);
            printf("< Output Saved: %S\r\n", strOutput.GetString());
        }
        else
        {
            throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
        }
    }
    catch (CMyException kException)
    {
        printf("! Error (%d): %S", kException.m_nLine, kException.m_strMessage.GetString());
    }
    catch (...)
    {
    }

    printf("\r\n");
    return 0;
}
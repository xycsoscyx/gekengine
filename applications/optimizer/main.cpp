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

struct MODEL
{
    std::vector<UINT16> m_aIndices;
    std::vector<VERTEX> m_aVertices;
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

void GetMeshes(const aiScene *pScene, const aiNode *pNode, const float4x4 &nParentTransform, std::multimap<CStringA, MODEL> &aModels, aabb &nAABB)
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

                MODEL kModel;
                for (UINT32 nFace = 0; nFace < pMesh->mNumFaces; nFace++)
                {
                    const aiFace &kFace = pMesh->mFaces[nFace];
                    if (kFace.mNumIndices == 3)
                    {
                        kModel.m_aIndices.push_back(kFace.mIndices[0]);
                        kModel.m_aIndices.push_back(kFace.mIndices[1]);
                        kModel.m_aIndices.push_back(kFace.mIndices[2]);
                    }
                }

                for (UINT32 nVertex = 0; nVertex < pMesh->mNumVertices; nVertex++)
                {
                    VERTEX kVertex;
                    kVertex.position.x = pMesh->mVertices[nVertex].x;
                    kVertex.position.y = pMesh->mVertices[nVertex].y;
                    kVertex.position.z = pMesh->mVertices[nVertex].z;
                    kVertex.position = (nTransform * float4(kVertex.position, 1.0f));
                    nAABB.Extend(kVertex.position);

                    if (pMesh->mTextureCoords && pMesh->mTextureCoords[0])
                    {
                        kVertex.texcoord.x = pMesh->mTextureCoords[0][nVertex].x;
                        kVertex.texcoord.y = pMesh->mTextureCoords[0][nVertex].y;
                    }

                    if (pMesh->mTangents)
                    {
                        kVertex.tangent.x = pMesh->mTangents[nVertex].x;
                        kVertex.tangent.y = pMesh->mTangents[nVertex].y;
                        kVertex.tangent.z = pMesh->mTangents[nVertex].z;
                        kVertex.tangent = (nTransform * kVertex.tangent);
                    }

                    if (pMesh->mBitangents)
                    {
                        kVertex.bitangent.x = pMesh->mBitangents[nVertex].x;
                        kVertex.bitangent.y = pMesh->mBitangents[nVertex].y;
                        kVertex.bitangent.z = pMesh->mBitangents[nVertex].z;
                        kVertex.bitangent = (nTransform * kVertex.bitangent);
                    }

                    if (pMesh->mNormals)
                    {
                        kVertex.normal.x = pMesh->mNormals[nVertex].x;
                        kVertex.normal.y = pMesh->mNormals[nVertex].y;
                        kVertex.normal.z = pMesh->mNormals[nVertex].z;
                        kVertex.normal = (nTransform * kVertex.normal);
                    }

                    kModel.m_aVertices.push_back(kVertex);
                }

                aModels.insert(std::make_pair(strMaterial, kModel));
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
            GetMeshes(pScene, pNode->mChildren[nChild], nTransform, aModels, nAABB);
        }
    }
}

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("GEK Mesh Optimizer\r\n");

    CStringW strInput;
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

    if (strInput.IsEmpty())
    {
        printf("Arguments:\r\n");
        printf("-input <input filename>\r\n");
        printf("-epsilon <face epsilon>\r\n");
        printf("-edge <partial edge threshold>\r\n");
        printf("-point <singular point threshold>\r\n");
        printf("-normal <normal edge threshold>\r\n");
        return -1;
    }
    else
    {
        printf("Input: %S\r\n", strInput.GetString());
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
        std::multimap<CStringA, MODEL> aScene;
        GetMeshes(pScene, pScene->mRootNode, float4x4(), aScene, nAABB);

        printf("< Num. Materials: %d\r\n", aScene.size());

        MODEL *pCollision = nullptr;
        MODEL *pOcclusion = nullptr;
        std::map<CStringA, MODEL> aMaterials;
        for (auto &kPair : aScene)
        {
            if (kPair.first.CompareNoCase("*collision") == 0)
            {
                pCollision = &kPair.second;
            }
            else if (kPair.first.CompareNoCase("*occlusion") == 0)
            {
                pOcclusion = &kPair.second;
            }
            else
            {
                MODEL &kModel = aMaterials[kPair.first];
                for (auto &nIndex : kPair.second.m_aIndices)
                {
                    kModel.m_aIndices.push_back(nIndex + kModel.m_aVertices.size());
                }

                kModel.m_aVertices.insert(kModel.m_aVertices.end(), kPair.second.m_aVertices.begin(), kPair.second.m_aVertices.end());
            }
        }

        CPathW kPath = strInput;
        kPath.RemoveExtension();

        FILE *pFile = nullptr;
        CStringW strOutput = (kPath.m_strPath + ".model.gek");
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

                printf("--< Num. Base Faces: %d\r\n", (kPair.second.m_aIndices.size() / 3));
                printf("--< Optimizing and Generating Tangent Frame\r\n");

                UINT32 nFirstVertex = aTotalVertices.size();
                UINT32 nFirstIndex = aTotalIndices.size();
                UINT32 nNumIndices = kPair.second.m_aIndices.size();
                fwrite(&nFirstVertex, sizeof(UINT32), 1, pFile);
                fwrite(&nFirstIndex, sizeof(UINT32), 1, pFile);
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);

                aTotalVertices.insert(aTotalVertices.end(), kPair.second.m_aVertices.begin(), kPair.second.m_aVertices.end());
                aTotalIndices.insert(aTotalIndices.end(), kPair.second.m_aIndices.begin(), kPair.second.m_aIndices.end());
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
            pFile = nullptr;
            printf("< Output Model Saved: %S\r\n", strOutput.GetString());
        }
        else
        {
            throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
        }

        if (pCollision)
        {
            strOutput = (kPath.m_strPath + ".collision.gek");
            _wfopen_s(&pFile, strOutput, L"wb");
            if (pFile != nullptr)
            {
                UINT32 nGEKX = *(UINT32 *)"GEKX";
                UINT16 nType = 1;
                UINT16 nVersion = 2;
                fwrite(&nGEKX, sizeof(UINT32), 1, pFile);
                fwrite(&nType, sizeof(UINT16), 1, pFile);
                fwrite(&nVersion, sizeof(UINT16), 1, pFile);
                fwrite(&nAABB, sizeof(aabb), 1, pFile);

                std::vector<float3> aVertices;
                for (auto &kVertex : pCollision->m_aVertices)
                {
                    aVertices.push_back(kVertex.position);
                }

                std::vector<float3> aOutputVertices;
                std::vector<UINT16> aOutputIndices;
                printf("--< Optimizing Collision Data\r\n");
                if (FAILED(GEKOptimizeMesh(&aVertices[0], aVertices.size(),
                                           &pCollision->m_aIndices[0], pCollision->m_aIndices.size(),
                                           aOutputVertices, aOutputIndices, nFaceEpsilon)))
                {
                    throw CMyException(__LINE__, L"Unable to optimize collision data");
                }

                UINT32 nNumVertices = aOutputVertices.size();
                fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
                printf("-< Num. Collision Vertices: %d\r\n", aOutputVertices.size());
                fwrite(&aOutputVertices[0], sizeof(float3), aOutputVertices.size(), pFile);

                UINT32 nNumIndices = aOutputIndices.size();
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
                printf("-< Num. Collision Indices: %d\r\n", aOutputIndices.size());
                fwrite(&aOutputIndices[0], sizeof(UINT16), aOutputIndices.size(), pFile);

                fclose(pFile);
                printf("< Output Collision Saved: %S\r\n", strOutput.GetString());
            }
            else
            {
                throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
            }
        }
        else
        {
            printf("< No Collision Data Found\r\n");
        }

        if (pOcclusion)
        {
            strOutput = (kPath.m_strPath + ".occlusion.gek");
            _wfopen_s(&pFile, strOutput, L"wb");
            if (pFile != nullptr)
            {
                UINT32 nGEKX = *(UINT32 *)"GEKX";
                UINT16 nType = 1;
                UINT16 nVersion = 2;
                fwrite(&nGEKX, sizeof(UINT32), 1, pFile);
                fwrite(&nType, sizeof(UINT16), 1, pFile);
                fwrite(&nVersion, sizeof(UINT16), 1, pFile);
                fwrite(&nAABB, sizeof(aabb), 1, pFile);

                std::vector<float3> aVertices;
                for (auto &kVertex : pOcclusion->m_aVertices)
                {
                    aVertices.push_back(kVertex.position);
                }

                std::vector<float3> aOutputVertices;
                std::vector<UINT16> aOutputIndices;
                printf("--< Optimizing Occlusion Data\r\n");
                if (FAILED(GEKOptimizeMesh(&aVertices[0], aVertices.size(),
                                           &pOcclusion->m_aIndices[0], pOcclusion->m_aIndices.size(),
                                           aOutputVertices, aOutputIndices, nFaceEpsilon)))
                {
                    throw CMyException(__LINE__, L"Unable to optimize occlusion data");
                }

                UINT32 nNumVertices = aOutputVertices.size();
                fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
                printf("-< Num. Occlusion Vertices: %d\r\n", aOutputVertices.size());
                fwrite(&aOutputVertices[0], sizeof(float3), aOutputVertices.size(), pFile);

                UINT32 nNumIndices = aOutputIndices.size();
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
                printf("-< Num. Occlusion Indices: %d\r\n", aOutputIndices.size());
                fwrite(&aOutputIndices[0], sizeof(UINT16), aOutputIndices.size(), pFile);

                fclose(pFile);
                printf("< Output Collision Saved: %S\r\n", strOutput.GetString());
            }
            else
            {
                throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
            }
        }
        else
        {
            printf("< No Occlusion Data Found\r\n");
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
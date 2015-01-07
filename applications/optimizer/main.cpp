#include "GEKMath.h"
#include "GEKShape.h"
#include "GEKUtility.h"
#include <atlpath.h>
#include <algorithm>
#include <map>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp.lib")

struct MODEL
{
    std::vector<UINT16> m_aIndices;
    std::vector<float3> m_aVertices;
    std::vector<float2> m_aTexCoords;
    std::vector<float3> m_aNormals;
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

    float4x4 nLocalTransform(*(float4x4 *)&pNode->mTransformation);
    nLocalTransform.Transpose();

    float4x4 nTransform(nLocalTransform * nParentTransform);
    float4x4 nRotation(quaternion(nTransform).GetInverse());
    float4x4 nInverseTransform(nTransform.GetInverse());
    if (pNode->mNumMeshes > 0)
    {
        if (pNode->mMeshes == nullptr)
        {
            throw CMyException(__LINE__, L"Invalid node mesh data");
        }

        for (UINT32 nMesh = 0; nMesh < pNode->mNumMeshes; ++nMesh)
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
                    throw CMyException(__LINE__, L"Mesh missing face information");
                }

                if (pMesh->mVertices == nullptr)
                {
                    throw CMyException(__LINE__, L"Mesh missing vertex information");
                }

                if (pMesh->mTextureCoords == nullptr || pMesh->mTextureCoords[0] == nullptr)
                {
                    throw CMyException(__LINE__, L"Mesh missing texcoord0 information");
                }

                if (pMesh->mNormals == nullptr)
                {
                    throw CMyException(__LINE__, L"Mesh missing normal information");
                }

                CStringA strMaterial;
                if (pScene->mMaterials != nullptr)
                {
                    const aiMaterial *pMaterial = pScene->mMaterials[pMesh->mMaterialIndex];
                    if (pMaterial != nullptr)
                    {
                        aiString strName;
                        pMaterial->Get(AI_MATKEY_NAME, strName);
                        CStringA strNameString = strName.C_Str();
                        strNameString.MakeLower();

                        aiString strDiffuse;
                        pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &strDiffuse);
                        CStringA strDiffuseString = strDiffuse.C_Str();
                        if (!strDiffuseString.IsEmpty())
                        {
                            strMaterial = strDiffuseString;
                        }
                    }
                }

                MODEL kModel;
                for (UINT32 nFace = 0; nFace < pMesh->mNumFaces; ++nFace)
                {
                    const aiFace &kFace = pMesh->mFaces[nFace];
                    if (kFace.mNumIndices == 3)
                    {
                        kModel.m_aIndices.push_back(kFace.mIndices[0]);
                        kModel.m_aIndices.push_back(kFace.mIndices[1]);
                        kModel.m_aIndices.push_back(kFace.mIndices[2]);
                    }
                }

                for (UINT32 nVertex = 0; nVertex < pMesh->mNumVertices; ++nVertex)
                {
                    float3 nPosition(pMesh->mVertices[nVertex].x,
                                     pMesh->mVertices[nVertex].y,
                                     pMesh->mVertices[nVertex].z);
                    nPosition = (nTransform * float4(nPosition, 1.0f));
                    kModel.m_aVertices.push_back(nPosition);
                    nAABB.Extend(nPosition);

                    float2 nTexCoord;
                    nTexCoord.x = pMesh->mTextureCoords[0][nVertex].x;
                    nTexCoord.y = pMesh->mTextureCoords[0][nVertex].y;
                    kModel.m_aTexCoords.push_back(nTexCoord);

                    float3 nNormal;
                    nNormal.x = pMesh->mNormals[nVertex].x;
                    nNormal.y = pMesh->mNormals[nVertex].y;
                    nNormal.z = pMesh->mNormals[nVertex].z;
                    nNormal = (nRotation * nNormal);

                    kModel.m_aNormals.push_back(nNormal.GetNormal());
                }

                if (!strMaterial.IsEmpty())
                {
                    aModels.insert(std::make_pair(strMaterial, kModel));
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

        for (UINT32 nChild = 0; nChild < pNode->mNumChildren; ++nChild)
        {
            GetMeshes(pScene, pNode->mChildren[nChild], nTransform, aModels, nAABB);
        }
    }
}

int wmain(int nNumArguments, wchar_t *astrArguments[], wchar_t *astrEnvironmentVariables)
{
    printf("GEK Mesh Optimizer\r\n");

    CStringW strInput;
    CStringW strOutput;
    bool bSmooth = false;
    float nSmoothAngle = 80.0f;
    for (int nArgument = 1; nArgument < nNumArguments; nArgument++)
    {
        if (_wcsicmp(astrArguments[nArgument], L"-input") == 0 && ++nArgument < nNumArguments)
        {
            strInput = astrArguments[nArgument];
        }
        else if (_wcsicmp(astrArguments[nArgument], L"-output") == 0 && ++nArgument < nNumArguments)
        {
            strOutput = astrArguments[nArgument];
        }
        else if (_wcsicmp(astrArguments[nArgument], L"-smooth") == 0)
        {
            bSmooth = true;
            if (++nArgument < nNumArguments)
            {
                nSmoothAngle = StrToFloat(astrArguments[nArgument]);
            }
        }
    }

    if (bSmooth)
    {
        printf(" Smoothing Angle: %f\r\n", nSmoothAngle);
    }
    else
    {
        printf(" Smoothing Disabled\r\n");
    }

    try
    {
        aiPropertyStore *pPropertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(pPropertyStore, AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_TANGENTS_AND_BITANGENTS | (bSmooth ? aiComponent_NORMALS : 0));
        if (bSmooth)
        {
            aiSetImportPropertyFloat(pPropertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, nSmoothAngle);
        }

        const aiScene *pScene = aiImportFileExWithProperties(CW2A(strInput, CP_UTF8), aiProcess_RemoveComponent | aiProcess_FlipUVs | aiProcess_TransformUVCoords, nullptr, pPropertyStore);
        if (pScene == nullptr)
        {
            throw CMyException(__LINE__, L"Unable to Load Input: %s", strInput.GetString());
        }

        if (pScene->mMeshes == nullptr)
        {
            throw CMyException(__LINE__, L"No meshes found in scene: %s", strInput.GetString());
        }

        aiApplyPostProcessing(pScene, aiProcess_FindInvalidData | aiProcess_Triangulate | aiProcess_RemoveRedundantMaterials);
        aiApplyPostProcessing(pScene, bSmooth ? aiProcess_GenSmoothNormals : aiProcess_GenNormals);
        aiApplyPostProcessing(pScene, aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices);
        aiReleasePropertyStore(pPropertyStore);

        aabb nAABB;
        std::multimap<CStringA, MODEL> aScene;
        GetMeshes(pScene, pScene->mRootNode, float4x4(), aScene, nAABB);
        printf("< Num. Materials: %d\r\n", aScene.size());

        std::unordered_map<CStringA, MODEL> aMaterials;
        for (auto &kPair : aScene)
        {
            MODEL *pModel = &aMaterials[kPair.first];
            for (auto &nIndex : kPair.second.m_aIndices)
            {
                pModel->m_aIndices.push_back(nIndex + pModel->m_aVertices.size());
            }

            pModel->m_aVertices.insert(pModel->m_aVertices.end(), kPair.second.m_aVertices.begin(), kPair.second.m_aVertices.end());
            pModel->m_aTexCoords.insert(pModel->m_aTexCoords.end(), kPair.second.m_aTexCoords.begin(), kPair.second.m_aTexCoords.end());
            pModel->m_aNormals.insert(pModel->m_aNormals.end(), kPair.second.m_aNormals.begin(), kPair.second.m_aNormals.end());
        }

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

            MODEL kFinal;
            for (auto &kPair : aMaterials)
            {
                CStringA strMaterial = (kPair.first);
                strMaterial.Replace("/", "\\");

                CPathA kPath = strMaterial;
                kPath.RemoveExtension();
                strMaterial = kPath.m_strPath;

                int nTexturesRoot = strMaterial.Find("\\textures\\");
                if (nTexturesRoot >= 0)
                {
                    strMaterial = strMaterial.Mid(nTexturesRoot + 10);
                }
                
                if (strMaterial.Right(9).CompareNoCase(".colormap") == 0)
                {
                    strMaterial = strMaterial.Left(strMaterial.GetLength() - 9);
                }

                fwrite(strMaterial.GetString(), (strMaterial.GetLength() + 1), 1, pFile);

                printf("-< Material: %s\r\n", strMaterial.GetString());
                printf("--< Num. Vertices: %d\r\n", kPair.second.m_aVertices.size());
                printf("--< Num. Indices: %d\r\n", kPair.second.m_aIndices.size());

                UINT32 nFirstVertex = kFinal.m_aVertices.size();
                UINT32 nFirstIndex = kFinal.m_aIndices.size();
                UINT32 nNumIndices = kPair.second.m_aIndices.size();
                fwrite(&nFirstVertex, sizeof(UINT32), 1, pFile);
                fwrite(&nFirstIndex, sizeof(UINT32), 1, pFile);
                fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);

                kFinal.m_aVertices.insert(kFinal.m_aVertices.end(), kPair.second.m_aVertices.begin(), kPair.second.m_aVertices.end());
                kFinal.m_aTexCoords.insert(kFinal.m_aTexCoords.end(), kPair.second.m_aTexCoords.begin(), kPair.second.m_aTexCoords.end());
                kFinal.m_aNormals.insert(kFinal.m_aNormals.end(), kPair.second.m_aNormals.begin(), kPair.second.m_aNormals.end());
                kFinal.m_aIndices.insert(kFinal.m_aIndices.end(), kPair.second.m_aIndices.begin(), kPair.second.m_aIndices.end());
            }

            UINT32 nNumVertices = kFinal.m_aVertices.size();
            fwrite(&nNumVertices, sizeof(UINT32), 1, pFile);
            printf("-< Num. Total Vertices: %d\r\n", kFinal.m_aVertices.size());
            fwrite(kFinal.m_aVertices.data(), sizeof(float3), kFinal.m_aVertices.size(), pFile);
            fwrite(kFinal.m_aTexCoords.data(), sizeof(float2), kFinal.m_aTexCoords.size(), pFile);
            fwrite(kFinal.m_aNormals.data(), sizeof(float3), kFinal.m_aNormals.size(), pFile);

            UINT32 nNumIndices = kFinal.m_aIndices.size();
            fwrite(&nNumIndices, sizeof(UINT32), 1, pFile);
            printf("-< Num. Total Indices: %d\r\n", kFinal.m_aIndices.size());
            fwrite(kFinal.m_aIndices.data(), sizeof(UINT16), kFinal.m_aIndices.size(), pFile);

            fclose(pFile);
            pFile = nullptr;
            printf("< Output Model Saved: %S\r\n", strOutput.GetString());
        }
        else
        {
            throw CMyException(__LINE__, L"! Unable to Save Output: %S\r\n", strOutput.GetString());
        }

        aiReleaseImport(pScene);
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
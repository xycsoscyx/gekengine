#include "GEK\Math\Common.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\Common.h"
#include <atlpath.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp.lib")

struct Vertex
{
    Gek::Math::Float3 position;
    Gek::Math::Float2 texCoord;
    Gek::Math::Float3 normal;
};

struct Model
{
    std::vector<UINT16> indexList;
    std::vector<Vertex> vertexList;
};

class OptimizerException
{
public:
    CStringW message;
    int line;

public:
    OptimizerException(int line, LPCWSTR format, ...)
        : line(line)
    {
        va_list variableList;
        va_start(variableList, format);
        message.FormatV(format, variableList);
        va_end(variableList);
    }
};

void GetMeshes(const aiScene *scene, const aiNode *node, const Gek::Math::Float4x4 &parentTransformation, std::multimap<CStringA, Model> &modelList, Gek::Shape::AlignedBox &alignedBox)
{
    if (node == nullptr)
    {
        throw OptimizerException(__LINE__, L"Invalid node encountered");
    }

    Gek::Math::Float4x4 localTransformation(*(Gek::Math::Float4x4 *)&node->mTransformation);
    localTransformation.transpose();

    Gek::Math::Float4x4 transformation(localTransformation * parentTransformation);
    Gek::Math::Float4x4 inverseRotation(Gek::Math::Quaternion(transformation).getInverse());
    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            throw OptimizerException(__LINE__, L"Invalid node mesh data");
        }

        for (UINT32 meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            UINT32 nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                throw OptimizerException(__LINE__, L"Invalid mesh index encountered: %d (of %d)", nodeMeshIndex, scene->mNumMeshes);
            }

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                if (mesh->mFaces == nullptr)
                {
                    throw OptimizerException(__LINE__, L"Mesh missing face information");
                }

                if (mesh->mVertices == nullptr)
                {
                    throw OptimizerException(__LINE__, L"Mesh missing vertex information");
                }

                if (mesh->mTextureCoords == nullptr || mesh->mTextureCoords[0] == nullptr)
                {
                    throw OptimizerException(__LINE__, L"Mesh missing texcoord0 information");
                }

                if (mesh->mNormals == nullptr)
                {
                    throw OptimizerException(__LINE__, L"Mesh missing normal information");
                }

                CStringA material;
                if (scene->mMaterials != nullptr)
                {
                    const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                    if (sceneMaterial != nullptr)
                    {
                        aiString sceneDiffuseMaterial;
                        sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                        CStringA diffuseMaterial = sceneDiffuseMaterial.C_Str();
                        if (!diffuseMaterial.IsEmpty())
                        {
                            material = diffuseMaterial;
                        }
                    }
                }

                Model model;
                for (UINT32 faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = mesh->mFaces[faceIndex];
                    if (face.mNumIndices == 3)
                    {
                        model.indexList.push_back(face.mIndices[0]);
                        model.indexList.push_back(face.mIndices[1]);
                        model.indexList.push_back(face.mIndices[2]);
                    }
                }

                for (UINT32 vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                {
                    Vertex vertex;
                    vertex.position = (transformation * Gek::Math::Float3(mesh->mVertices[vertexIndex].x,
                                                                          mesh->mVertices[vertexIndex].y,
                                                                          mesh->mVertices[vertexIndex].z));
                    alignedBox.extend(vertex.position);

                    vertex.texCoord.x = mesh->mTextureCoords[0][vertexIndex].x;
                    vertex.texCoord.y = mesh->mTextureCoords[0][vertexIndex].y;

                    Gek::Math::Float3 normal;
                    vertex.normal.x = mesh->mNormals[vertexIndex].x;
                    vertex.normal.y = mesh->mNormals[vertexIndex].y;
                    vertex.normal.z = mesh->mNormals[vertexIndex].z;
                    vertex.normal = (inverseRotation * normal);

                    model.vertexList.push_back(vertex);
                }

                if (!material.IsEmpty())
                {
                    modelList.insert(std::make_pair(material, model));
                }
            }
        }
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            throw OptimizerException(__LINE__, L"Invalid node children data");
        }

        for (UINT32 childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            GetMeshes(scene, node->mChildren[childIndex], transformation, modelList, alignedBox);
        }
    }
}

int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Mesh Optimizer\r\n");

    CStringW fileNameInput;
    CStringW fileNameOutput;

    bool flip = false;
    bool enableSmoothing = false;
    float smoothingAngle = 80.0f;
    for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
    {
        if (_wcsicmp(argumentList[argumentIndex], L"-input") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameInput = argumentList[argumentIndex];
        }
        else if (_wcsicmp(argumentList[argumentIndex], L"-output") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameOutput = argumentList[argumentIndex];
        }
        else if (_wcsicmp(argumentList[argumentIndex], L"-flip") == 0)
        {
            flip = true;
        }
        else if (_wcsicmp(argumentList[argumentIndex], L"-smooth") == 0)
        {
            enableSmoothing = true;
            if (++argumentIndex < argumentCount)
            {
                smoothingAngle = Gek::String::toFloat(argumentList[argumentIndex]);
            }
        }
    }

    if (enableSmoothing)
    {
        printf(" Smoothing Angle: %f\r\n", smoothingAngle);
    }
    else
    {
        printf(" Smoothing Disabled\r\n");
    }

    try
    {
        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, aiComponent_COLORS | aiComponent_TANGENTS_AND_BITANGENTS | (enableSmoothing ? aiComponent_NORMALS : 0));
        if (enableSmoothing)
        {
            aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
        }

        const aiScene *scene = aiImportFileExWithProperties(CW2A(fileNameInput, CP_UTF8), aiProcess_RemoveComponent | aiProcess_FlipUVs | aiProcess_TransformUVCoords, nullptr, propertyStore);
        if (scene == nullptr)
        {
            throw OptimizerException(__LINE__, L"Unable to Load Input: %s", fileNameInput.GetString());
        }

        if (scene->mMeshes == nullptr)
        {
            throw OptimizerException(__LINE__, L"No meshes found in scene: %s", fileNameInput.GetString());
        }

        aiApplyPostProcessing(scene, aiProcess_FindInvalidData | aiProcess_Triangulate | aiProcess_RemoveRedundantMaterials | (flip ? aiProcess_FlipWindingOrder : 0));
        aiApplyPostProcessing(scene, enableSmoothing ? aiProcess_GenSmoothNormals : aiProcess_GenNormals);
        aiApplyPostProcessing(scene, aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices);
        aiReleasePropertyStore(propertyStore);

        Gek::Shape::AlignedBox alignedBox;
        std::multimap<CStringA, Model> modelList;
        GetMeshes(scene, scene->mRootNode, Gek::Math::Float4x4(), modelList, alignedBox);
        printf("< Num. Materials: %d\r\n", modelList.size());

        std::unordered_map<CStringA, Model> materialModelList;
        for (auto &model : modelList)
        {
            Model &materialModel = materialModelList[model.first];
            for (auto &nIndex : model.second.indexList)
            {
                materialModel.indexList.push_back(UINT16(nIndex + materialModel.vertexList.size()));
            }

            materialModel.vertexList.insert(materialModel.vertexList.end(), model.second.vertexList.begin(), model.second.vertexList.end());
        }

        FILE *file = nullptr;
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file != nullptr)
        {
            UINT32 nGEKX = *(UINT32 *)"GEKX";
            UINT16 nType = 0;
            UINT16 nVersion = 2;
            UINT32 materialCount = materialModelList.size();
            fwrite(&nGEKX, sizeof(UINT32), 1, file);
            fwrite(&nType, sizeof(UINT16), 1, file);
            fwrite(&nVersion, sizeof(UINT16), 1, file);
            fwrite(&alignedBox, sizeof(Gek::Shape::AlignedBox), 1, file);
            fwrite(&materialCount, sizeof(UINT32), 1, file);

            Model finalModelData;
            for (auto &materialModel : materialModelList)
            {
                CStringA material = (materialModel.first);
                material.Replace("/", "\\");

                CPathA kPath(material);
                kPath.RemoveExtension();
                material = LPCSTR(kPath);

                int texturesPathIndex = material.Find("\\textures\\");
                if (texturesPathIndex >= 0)
                {
                    material = material.Mid(texturesPathIndex + 10);
                }
                
                if (material.Right(9).CompareNoCase(".colormap") == 0)
                {
                    material = material.Left(material.GetLength() - 9);
                }

                fwrite(material.GetString(), (material.GetLength() + 1), 1, file);

                printf("-< Material: %s\r\n", material.GetString());
                printf("--< Num. Vertices: %d\r\n", materialModel.second.vertexList.size());
                printf("--< Num. Indices: %d\r\n", materialModel.second.indexList.size());

                UINT32 firstVertex = finalModelData.vertexList.size();
                UINT32 firstIndex = finalModelData.indexList.size();
                UINT32 indexCount = materialModel.second.indexList.size();
                fwrite(&firstVertex, sizeof(UINT32), 1, file);
                fwrite(&firstIndex, sizeof(UINT32), 1, file);
                fwrite(&indexCount, sizeof(UINT32), 1, file);

                finalModelData.vertexList.insert(finalModelData.vertexList.end(), materialModel.second.vertexList.begin(), materialModel.second.vertexList.end());
                finalModelData.indexList.insert(finalModelData.indexList.end(), materialModel.second.indexList.begin(), materialModel.second.indexList.end());
            }

            UINT32 vertexCount = finalModelData.vertexList.size();
            fwrite(&vertexCount, sizeof(UINT32), 1, file);
            printf("-< Num. Total Vertices: %d\r\n", finalModelData.vertexList.size());
            fwrite(finalModelData.vertexList.data(), sizeof(Vertex), finalModelData.vertexList.size(), file);

            UINT32 indexCount = finalModelData.indexList.size();
            fwrite(&indexCount, sizeof(UINT32), 1, file);
            printf("-< Num. Total Indices: %d\r\n", finalModelData.indexList.size());
            fwrite(finalModelData.indexList.data(), sizeof(UINT16), finalModelData.indexList.size(), file);

            fclose(file);
            file = nullptr;
            printf("< Output Model Saved: %S\r\n", fileNameOutput.GetString());
        }
        else
        {
            throw OptimizerException(__LINE__, L"[error] Unable to Save Output: %S\r\n", fileNameOutput.GetString());
        }

        aiReleaseImport(scene);
    }
    catch (OptimizerException exception)
    {
        printf("[error] Error (%d): %S", exception.line, exception.message.GetString());
    }
    catch (...)
    {
    }

    printf("\r\n");
    return 0;
}
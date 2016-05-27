#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\String.h"
#include <atlpath.h>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <map>

#include <Newton.h>
#pragma comment(lib, "newton.lib")

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp.lib")

class OptimizerException
{
public:
    CStringW message;
    int line;

public:
    OptimizerException(int line, const wchar_t *formatting, ...)
        : line(line)
    {
        va_list variableList;
        va_start(variableList, formatting);
        message.FormatV(formatting, variableList);
        va_end(variableList);
    }
};

namespace Gek
{
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

    void GetMeshes(const aiScene *scene, const aiNode *node, std::unordered_map<CStringA, std::list<Model>> &modelList, Gek::Shapes::AlignedBox &boundingBox)
    {
        if (node == nullptr)
        {
            throw OptimizerException(__LINE__, L"Invalid node encountered");
        }

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
                    throw OptimizerException(__LINE__, L"Invalid mesh index encountered: %v (of %v)", nodeMeshIndex, scene->mNumMeshes);
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
                        else
                        {
                            printf("(Mesh %d) Invalid Face Found: %d (%d vertices)\r\n", meshIndex, faceIndex, face.mNumIndices);
                        }
                    }

                    for (UINT32 vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                    {
                        Vertex vertex;
                        vertex.position.set(
                            mesh->mVertices[vertexIndex].x,
                            mesh->mVertices[vertexIndex].y,
                            mesh->mVertices[vertexIndex].z);
                        boundingBox.extend(vertex.position);

                        if (mesh->mTextureCoords[0])
                        {
                            vertex.texCoord.set(
                                mesh->mTextureCoords[0][vertexIndex].x,
                                mesh->mTextureCoords[0][vertexIndex].y);
                        }

                        if (mesh->mNormals)
                        {
                            vertex.normal.set(
                                mesh->mNormals[vertexIndex].x,
                                mesh->mNormals[vertexIndex].y,
                                mesh->mNormals[vertexIndex].z);
                        }

                        model.vertexList.push_back(vertex);
                    }

                    if (!material.IsEmpty())
                    {
                        modelList[material].push_back(model);
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
                GetMeshes(scene, node->mChildren[childIndex], modelList, boundingBox);
            }
        }
    }

    void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
    {
        FILE *file = (FILE *)serializeHandle;
        fwrite(buffer, 1, size, file);
    }
}; // namespace Gek

int wmain(int argumentCount, wchar_t *argumentList[], wchar_t *environmentVariableList)
{
    printf("GEK Model Converter\r\n");

    CStringW fileNameInput;
    CStringW fileNameOutput;
    CStringW mode("model");

    bool flipCoords = false;
    bool flipWinding = false;
    bool generateNormals = false;
    bool smoothNormals = false;
    float smoothingAngle = 80.0f;
    for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
    {
        CStringW argument(argumentList[argumentIndex]);

        int position = 0;
        CStringW operation(argument.Tokenize(L":", position));
        if (operation.CompareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameInput = argumentList[argumentIndex];
        }
        else if (operation.CompareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
        {
            fileNameOutput = argumentList[argumentIndex];
        }
        else if (operation.CompareNoCase(L"-mode") == 0)
        {
            mode = argument.Tokenize(L":", position);
        }
        else if (operation.CompareNoCase(L"-flipCoords") == 0)
        {
            flipCoords = true;
        }
        else if (operation.CompareNoCase(L"-flipWinding") == 0)
        {
            flipWinding = true;
        }
        else if (operation.CompareNoCase(L"-generateNormals") == 0)
        {
            generateNormals = true;
        }
        else if (operation.CompareNoCase(L"-smoothNormals") == 0)
        {
            smoothNormals = true;
            smoothingAngle = Gek::String::to<float>(argument.Tokenize(L":", position));
        }
    }

    try
    {
        int notRequiredComponents =
            aiComponent_TANGENTS_AND_BITANGENTS |
            aiComponent_COLORS |
            aiComponent_BONEWEIGHTS |
            aiComponent_ANIMATIONS |
            aiComponent_LIGHTS |
            aiComponent_CAMERAS |
            0;

        unsigned int importFlags =
            (flipWinding ? aiProcess_FlipWindingOrder : 0) |
            aiProcess_RemoveComponent | // remove extra data that is not required
            aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
            aiProcess_Triangulate | // triangulate polygons with more than 3 edges
            aiProcess_SortByPType | // make ‘clean’ meshes which consist of a single typ of primitives
            aiProcess_ValidateDataStructure | // perform a full validation of the loader’s output
            aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
            aiProcess_RemoveRedundantMaterials | // remove redundant materials
            aiProcess_FindDegenerates | // remove degenerated polygons from the import
            aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
            aiProcess_OptimizeMeshes | // join small meshes, if possible;
            0;

        unsigned int postProcessFlags =
            aiProcess_PreTransformVertices | // pretransform the vertices by the a local node matrices
            aiProcess_JoinIdenticalVertices | // join identical vertices / optimize indexing
            aiProcess_ValidateDataStructure | // perform a full validation of the loader’s output
            aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
            0;

        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        if (mode.CompareNoCase(L"model") == 0)
        {
            importFlags |= aiProcess_GenUVCoords; // convert spherical, cylindrical, box and planar mapping to proper UVs
            importFlags |= aiProcess_TransformUVCoords; // preprocess UV transformations (scaling, translation …)
            if (generateNormals)
            {
                notRequiredComponents |= aiComponent_NORMALS; // remove normals component since we are generating them
                if (smoothNormals)
                {
                    aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
                    postProcessFlags |= aiProcess_GenSmoothNormals; // generate smoothed normal vectors
                }
                else
                {
                    postProcessFlags |= aiProcess_GenNormals; // generate normal vectors
                }
            }
                
            aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
            if (flipCoords)
            {
                importFlags |= aiProcess_FlipUVs;
            }
        }
        else
        {
            notRequiredComponents |= aiComponent_TEXCOORDS; // we don't need texture coordinates for collision information
            notRequiredComponents |= aiComponent_NORMALS; // we don't need normals for collision information
        }

        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        const aiScene* scene = aiImportFileExWithProperties(CW2A(fileNameInput, CP_UTF8), importFlags, NULL, propertyStore);
        if (scene == nullptr)
        {
            throw OptimizerException(__LINE__, L"Unable to Load Input: %v", fileNameInput.GetString());
        }

        if (scene->mMeshes == nullptr)
        {
            throw OptimizerException(__LINE__, L"No meshes found in scene: %v", fileNameInput.GetString());
        }

        aiApplyPostProcessing(scene, postProcessFlags);

        Gek::Shapes::AlignedBox boundingBox;
        std::unordered_map<CStringA, std::list<Gek::Model>> modelListUTF8;
        Gek::GetMeshes(scene, scene->mRootNode, modelListUTF8, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        std::unordered_map<CStringW, std::list<Gek::Model>> modelList;
        for (auto &material : modelListUTF8)
        {
            CStringW materialName = CA2W(material.first, CP_UTF8);
            materialName.Replace(L"/", L"\\");

            CPathW materialPath(materialName);
            materialPath.RemoveExtension();
            materialName = const wchar_t *(materialPath);

            int texturesPathIndex = materialName.Find(L"\\textures\\");
            if (texturesPathIndex >= 0)
            {
                materialName = materialName.Mid(texturesPathIndex + 10);
            }

            if (materialName.Right(9).CompareNoCase(L".colormap") == 0)
            {
                materialName = materialName.Left(materialName.GetLength() - 9);
            }
            else if (materialName.Right(7).CompareNoCase(L".albedo") == 0)
            {
                materialName = materialName.Left(materialName.GetLength() - 7);
            }
            else if (materialName.Right(2).CompareNoCase(L"_a") == 0)
            {
                materialName = materialName.Left(materialName.GetLength() - 2);
            }

            CPathW fileSpecifier(materialName);
            fileSpecifier.StripPath();

            CPathW folderName(materialName);
            folderName.RemoveFileSpec();
            folderName.StripPath();

            if (fileSpecifier.m_strPath.CompareNoCase(folderName.m_strPath) == 0)
            {
                materialPath.m_strPath = materialName;
                materialPath.RemoveFileSpec();
                materialName = const wchar_t *(materialPath);
            }

            modelList[materialName] = material.second;
        }

        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);
        if (mode.CompareNoCase(L"model") == 0)
        {
            std::unordered_map<CStringW, Gek::Model> sortedModelList;
            for (auto &material : modelList)
            {
                Gek::Model &sortedModel = sortedModelList[material.first];
                for (auto &model : material.second)
                {
                    for (auto &index : model.indexList)
                    {
                        sortedModel.indexList.push_back(UINT16(index + sortedModel.vertexList.size()));
                    }

                    sortedModel.vertexList.insert(sortedModel.vertexList.end(), model.vertexList.begin(), model.vertexList.end());
                }
            }

            FILE *file = nullptr;
            _wfopen_s(&file, fileNameOutput, L"wb");
            if (file != nullptr)
            {
                UINT32 gekMagic = *(UINT32 *)"GEKX";
                UINT16 gekModelType = 0;
                UINT16 gekModelVersion = 3;
                UINT32 modelCount = sortedModelList.size();
                fwrite(&gekMagic, sizeof(UINT32), 1, file);
                fwrite(&gekModelType, sizeof(UINT16), 1, file);
                fwrite(&gekModelVersion, sizeof(UINT16), 1, file);
                fwrite(&boundingBox, sizeof(Gek::Shapes::AlignedBox), 1, file);
                fwrite(&modelCount, sizeof(UINT32), 1, file);

                printf("> Num. Models: %d\r\n", modelCount);
                for (auto &model : sortedModelList)
                {
                    CStringW materialName = model.first;
                    fwrite(materialName.GetString(), ((materialName.GetLength() + 1) * sizeof(WCHAR)), 1, file);

                    UINT32 vertexCount = model.second.vertexList.size();
                    fwrite(&vertexCount, sizeof(UINT32), 1, file);
                    fwrite(model.second.vertexList.data(), sizeof(Gek::Vertex), model.second.vertexList.size(), file);

                    UINT32 indexCount = model.second.indexList.size();
                    fwrite(&indexCount, sizeof(UINT32), 1, file);
                    fwrite(model.second.indexList.data(), sizeof(UINT16), model.second.indexList.size(), file);

                    printf("-  %S\r\n", materialName.GetString());
                    printf("    %d vertices\r\n", model.second.vertexList.size());
                    printf("    %d indices\r\n", model.second.indexList.size());
                }

                fclose(file);
            }
            else
            {
                throw OptimizerException(__LINE__, L"[error] Unable to Save Output: %v\r\n", fileNameOutput.GetString());
            }
        }
        else  if (mode.CompareNoCase(L"hull") == 0)
        {
            NewtonWorld *newtonWorld = NewtonCreate();
            std::vector<Gek::Math::Float3> pointCloudList;
            for (auto &material : modelList)
            {
                for (auto &model : material.second)
                {
                    for (auto &index : model.indexList)
                    {
                        pointCloudList.push_back(model.vertexList[index].position);
                    }
                }
            }

            printf("> Num. Points: %d\r\n", pointCloudList.size());
            NewtonCollision *newtonCollision = NewtonCreateConvexHull(newtonWorld, pointCloudList.size(), pointCloudList[0].data, sizeof(Gek::Math::Float3), 0.025f, 0, Gek::Math::Float4x4().data);
            if (newtonCollision)
            {
                FILE *file = nullptr;
                _wfopen_s(&file, fileNameOutput, L"wb");
                if (file != nullptr)
                {
                    UINT32 gekMagic = *(UINT32 *)"GEKX";
                    UINT16 gekModelType = 1;
                    UINT16 gekModelVersion = 0;
                    fwrite(&gekMagic, sizeof(UINT32), 1, file);
                    fwrite(&gekModelType, sizeof(UINT16), 1, file);
                    fwrite(&gekModelVersion, sizeof(UINT16), 1, file);

                    NewtonCollisionSerialize(newtonWorld, newtonCollision, Gek::serializeCollision, file);
                    fclose(file);
                }

                NewtonDestroyCollision(newtonCollision);
            }
            else
            {
                throw OptimizerException(__LINE__, L"[error] Unable to Serialize Convex Hull: %v\r\n", fileNameOutput.GetString());
            }

            NewtonDestroy(newtonWorld);
        }
        else  if (mode.CompareNoCase(L"tree") == 0)
        {
            printf("> Num. Materials: %d\r\n", modelList.size());

            NewtonWorld *newtonWorld = NewtonCreate();
            NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
            if (newtonCollision != nullptr)
            {
                int materialIdentifier = 0;
                NewtonTreeCollisionBeginBuild(newtonCollision);
                for (auto &material : modelList)
                {
                    printf("-  %S: %d models\r\n", material.first.GetString(), material.second.size());
                    for (auto &model : material.second)
                    {
                        printf("-    %d vertices\r\n", model.vertexList.size());
                        printf("     %d indices\r\n", model.indexList.size());

                        std::vector<Gek::Math::Float3> faceVertexList;
                        auto &indexList = model.indexList;
                        auto &vertexList = model.vertexList;
                        for (auto &index : indexList)
                        {
                            faceVertexList.push_back(vertexList[index].position);
                        }

                        for (UINT32 index = 0; index < faceVertexList.size(); index += 3)
                        {
                            NewtonTreeCollisionAddFace(newtonCollision, 3, faceVertexList[index].data, sizeof(Gek::Math::Float3), materialIdentifier);
                        }
                    }

                    ++materialIdentifier;
                }

                NewtonTreeCollisionEndBuild(newtonCollision, 1);

                FILE *file = nullptr;
                _wfopen_s(&file, fileNameOutput, L"wb");
                if (file != nullptr)
                {
                    UINT32 gekMagic = *(UINT32 *)"GEKX";
                    UINT16 gekModelType = 2;
                    UINT16 gekModelVersion = 0;
                    UINT32 materialCount = modelList.size();
                    fwrite(&gekMagic, sizeof(UINT32), 1, file);
                    fwrite(&gekModelType, sizeof(UINT16), 1, file);
                    fwrite(&gekModelVersion, sizeof(UINT16), 1, file);
                    fwrite(&materialCount, sizeof(UINT32), 1, file);
                    for (auto &material : modelList)
                    {
                        fwrite(material.first.GetString(), ((material.first.GetLength() + 1) * sizeof(wchar_t)), 1, file);
                    }

                    NewtonCollisionSerialize(newtonWorld, newtonCollision, Gek::serializeCollision, file);
                    fclose(file);
                }
                else
                {
                    throw OptimizerException(__LINE__, L"[error] Unable to Serialize Tree: v%\r\n", fileNameOutput.GetString());
                }

                NewtonDestroyCollision(newtonCollision);
            }
            else
            {
                throw OptimizerException(__LINE__, L"[error] Unable to Create Tree Collision\r\n");
            }

            NewtonDestroy(newtonWorld);
        }
        else
        {
            throw OptimizerException(__LINE__, L"[error] Invalid conversion mode specified: %s", mode.GetString());
        }
    }
    catch (OptimizerException exception)
    {
        printf("[error] Error (%d): %S", exception.line, exception.message.GetString());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}
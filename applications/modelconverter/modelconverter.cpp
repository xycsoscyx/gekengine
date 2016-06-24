#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

#include <Newton.h>
#pragma comment(lib, "newton.lib")

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma comment(lib, "assimp.lib")

using namespace Gek;

struct Vertex
{
    Math::Float3 position;
    Math::Float2 texCoord;
    Math::Float3 normal;
};

struct Model
{
    std::vector<uint16_t> indexList;
    std::vector<Vertex> vertexList;
};

void getMeshes(const aiScene *scene, const aiNode *node, std::unordered_map<StringUTF8, std::list<Model>> &modelList, Shapes::AlignedBox &boundingBox)
{
    GEK_CHECK_CONDITION(node == nullptr, Trace::Exception, "Missing node data");
    if (node->mNumMeshes > 0)
    {
        GEK_CHECK_CONDITION(node->mMeshes == nullptr, Trace::Exception, "Node missing mesh data");
        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            GEK_CHECK_CONDITION(nodeMeshIndex >= scene->mNumMeshes, Trace::Exception, "Node mesh index out of range : %v (of %v)", nodeMeshIndex, scene->mNumMeshes);

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                GEK_CHECK_CONDITION(mesh->mFaces == nullptr, Trace::Exception, "Mesh missing face data");
                GEK_CHECK_CONDITION(mesh->mVertices == nullptr, Trace::Exception, "Mesh missing vertex data");

                StringUTF8 material;
                if (scene->mMaterials != nullptr)
                {
                    const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                    if (sceneMaterial != nullptr)
                    {
                        aiString sceneDiffuseMaterial;
                        sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                        if (sceneDiffuseMaterial.length > 0)
                        {
                            material = sceneDiffuseMaterial.C_Str();;
                        }
                    }
                }

                Model model;
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
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

                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
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

                if (!material.empty())
                {
                    modelList[material].push_back(model);
                }
            }
        }
    }

    if (node->mNumChildren > 0)
    {
        GEK_CHECK_CONDITION(node->mChildren == nullptr, Trace::Exception, "Node missing child data");
        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            getMeshes(scene, node->mChildren[childIndex], modelList, boundingBox);
        }
    }
}

void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fwrite(buffer, 1, size, file);
}

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    try
    {
        printf("GEK Model Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
        String mode(L"model");

        bool flipCoords = false;
        bool flipWinding = false;
        bool generateNormals = false;
        bool smoothNormals = false;
        float smoothingAngle = 80.0f;
        for (int argumentIndex = 1; argumentIndex < argumentCount; argumentIndex++)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            GEK_CHECK_CONDITION(arguments.empty(), Trace::Exception, "Invalid argument encountered: %v", argumentList[argumentIndex]);
            if (arguments[0].compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-mode") == 0)
            {
                GEK_CHECK_CONDITION(arguments.size() != 2, Trace::Exception, "Invalid values specified for mode");
                mode = arguments[1];
            }
            else if (arguments[0].compareNoCase(L"-flipCoords") == 0)
            {
                flipCoords = true;
            }
            else if (arguments[0].compareNoCase(L"-flipWinding") == 0)
            {
                flipWinding = true;
            }
            else if (arguments[0].compareNoCase(L"-generateNormals") == 0)
            {
                generateNormals = true;
            }
            else if (arguments[0].compareNoCase(L"-smoothNormals") == 0)
            {
                GEK_CHECK_CONDITION(arguments.size() != 2, Trace::Exception, "Invalid values specified for smoothNormals");

                smoothNormals = true;
                smoothingAngle = arguments[1];
            }
        }

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
        if (mode.compareNoCase(L"model") == 0)
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
        const aiScene* scene = aiImportFileExWithProperties(StringUTF8(fileNameInput), importFlags, nullptr, propertyStore);
        GEK_CHECK_CONDITION(scene == nullptr, Trace::Exception, "Unable to Load Input: %v", fileNameInput);
        GEK_CHECK_CONDITION(scene->mMeshes == nullptr, Trace::Exception, "No meshes found in scene: %v", fileNameInput);

        aiApplyPostProcessing(scene, postProcessFlags);

        Shapes::AlignedBox boundingBox;
        std::unordered_map<StringUTF8, std::list<Model>> modelListUTF8;
        getMeshes(scene, scene->mRootNode, modelListUTF8, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        std::unordered_map<String, std::list<Model>> modelList;
        for (auto &material : modelListUTF8)
        {
            String materialName(material.first);
            materialName.replace(L"/", L"\\");
            materialName = FileSystem::Path(materialName).replace_extension().generic_wstring();

            int texturesPathIndex = materialName.find(L"\\textures\\");
            if (texturesPathIndex != std::string::npos)
            {
                materialName = materialName.subString(texturesPathIndex + 10);
            }

            if (materialName.subString(materialName.length() - 9).compareNoCase(L".colormap") == 0)
            {
                materialName = materialName.subString(materialName.length() - 9);
            }
            else if (materialName.subString(materialName.length() - 7).compareNoCase(L".albedo") == 0)
            {
                materialName = materialName.subString(materialName.length() - 7);
            }
            else if (materialName.subString(materialName.length() - 2).compareNoCase(L"_a") == 0)
            {
                materialName = materialName.subString(materialName.length() - 2);
            }

            auto fileSpecifier = FileSystem::Path(materialName).filename();
            auto folderName = FileSystem::Path(materialName).remove_filename().filename();

            if (fileSpecifier == folderName)
            {
                materialName = FileSystem::Path(materialName).remove_filename().generic_wstring();
            }

            modelList[materialName] = material.second;
        }

        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);
        if (mode.compareNoCase(L"model") == 0)
        {
            std::unordered_map<String, Model> sortedModelList;
            for (auto &material : modelList)
            {
                Model &sortedModel = sortedModelList[material.first];
                for (auto &model : material.second)
                {
                    for (auto &index : model.indexList)
                    {
                        sortedModel.indexList.push_back(uint16_t(index + sortedModel.vertexList.size()));
                    }

                    sortedModel.vertexList.insert(sortedModel.vertexList.end(), model.vertexList.begin(), model.vertexList.end());
                }
            }

            FILE *file = nullptr;
            _wfopen_s(&file, fileNameOutput, L"wb");
            GEK_CHECK_CONDITION(file == nullptr, Trace::Exception, "Unable to open output file: %v", fileNameOutput);

            uint32_t gekMagic = *(uint32_t *)"GEKX";
            uint16_t gekModelType = 0;
            uint16_t gekModelVersion = 3;
            uint32_t modelCount = sortedModelList.size();
            fwrite(&gekMagic, sizeof(uint32_t), 1, file);
            fwrite(&gekModelType, sizeof(uint16_t), 1, file);
            fwrite(&gekModelVersion, sizeof(uint16_t), 1, file);
            fwrite(&boundingBox, sizeof(Shapes::AlignedBox), 1, file);
            fwrite(&modelCount, sizeof(uint32_t), 1, file);

            printf("> Num. Models: %d\r\n", modelCount);
            for (auto &model : sortedModelList)
            {
                String materialName(model.first);
                fwrite(materialName.c_str(), ((materialName.length() + 1) * sizeof(wchar_t)), 1, file);

                uint32_t vertexCount = model.second.vertexList.size();
                fwrite(&vertexCount, sizeof(uint32_t), 1, file);
                fwrite(model.second.vertexList.data(), sizeof(Vertex), model.second.vertexList.size(), file);

                uint32_t indexCount = model.second.indexList.size();
                fwrite(&indexCount, sizeof(uint32_t), 1, file);
                fwrite(model.second.indexList.data(), sizeof(uint16_t), model.second.indexList.size(), file);

                printf("-  %S\r\n", materialName.c_str());
                printf("    %d vertices\r\n", model.second.vertexList.size());
                printf("    %d indices\r\n", model.second.indexList.size());
            }

            fclose(file);
        }
        else  if (mode.compareNoCase(L"hull") == 0)
        {
            NewtonWorld *newtonWorld = NewtonCreate();
            std::vector<Math::Float3> pointCloudList;
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
            NewtonCollision *newtonCollision = NewtonCreateConvexHull(newtonWorld, pointCloudList.size(), pointCloudList[0].data, sizeof(Math::Float3), 0.025f, 0, Math::Float4x4().data);
            GEK_CHECK_CONDITION(newtonCollision == nullptr, Trace::Exception, "Unable to create convex hull");

            FILE *file = nullptr;
            _wfopen_s(&file, fileNameOutput, L"wb");
            GEK_CHECK_CONDITION(file == nullptr, Trace::Exception, "Unable to open output file: %v", fileNameOutput);

            uint32_t gekMagic = *(uint32_t *)"GEKX";
            uint16_t gekModelType = 1;
            uint16_t gekModelVersion = 0;
            fwrite(&gekMagic, sizeof(uint32_t), 1, file);
            fwrite(&gekModelType, sizeof(uint16_t), 1, file);
            fwrite(&gekModelVersion, sizeof(uint16_t), 1, file);

            NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
            fclose(file);
            NewtonDestroyCollision(newtonCollision);
            NewtonDestroy(newtonWorld);
        }
        else  if (mode.compareNoCase(L"tree") == 0)
        {
            printf("> Num. Materials: %d\r\n", modelList.size());

            NewtonWorld *newtonWorld = NewtonCreate();
            NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
            GEK_CHECK_CONDITION(newtonCollision == nullptr, Trace::Exception, "Unable to create convex hull");

            int materialIdentifier = 0;
            NewtonTreeCollisionBeginBuild(newtonCollision);
            for (auto &material : modelList)
            {
                printf("-  %S: %d models\r\n", material.first.c_str(), material.second.size());
                for (auto &model : material.second)
                {
                    printf("-    %d vertices\r\n", model.vertexList.size());
                    printf("     %d indices\r\n", model.indexList.size());

                    std::vector<Math::Float3> faceVertexList;
                    auto &indexList = model.indexList;
                    auto &vertexList = model.vertexList;
                    for (auto &index : indexList)
                    {
                        faceVertexList.push_back(vertexList[index].position);
                    }

                    for (uint32_t index = 0; index < faceVertexList.size(); index += 3)
                    {
                        NewtonTreeCollisionAddFace(newtonCollision, 3, faceVertexList[index].data, sizeof(Math::Float3), materialIdentifier);
                    }
                }

                ++materialIdentifier;
            }

            NewtonTreeCollisionEndBuild(newtonCollision, 1);

            FILE *file = nullptr;
            _wfopen_s(&file, fileNameOutput, L"wb");
            GEK_CHECK_CONDITION(file == nullptr, Trace::Exception, "Unable to open output file: %v", fileNameOutput);

            uint32_t gekMagic = *(uint32_t *)"GEKX";
            uint16_t gekModelType = 2;
            uint16_t gekModelVersion = 0;
            uint32_t materialCount = modelList.size();
            fwrite(&gekMagic, sizeof(uint32_t), 1, file);
            fwrite(&gekModelType, sizeof(uint16_t), 1, file);
            fwrite(&gekModelVersion, sizeof(uint16_t), 1, file);
            fwrite(&materialCount, sizeof(uint32_t), 1, file);
            for (auto &material : modelList)
            {
                fwrite(material.first.c_str(), ((material.first.length() + 1) * sizeof(wchar_t)), 1, file);
            }

            NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
            fclose(file);
            NewtonDestroyCollision(newtonCollision);
            NewtonDestroy(newtonWorld);
        }
        else
        {
            GEK_THROW_EXCEPTION(Trace::Exception, "Invalid conversion mode specified: %v", mode);
        }
    }
    catch (const Exception &exception)
    {
        printf("[error] Error (%d): %s", exception.at(), exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}
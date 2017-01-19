#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include <unordered_map>
#include <algorithm>
#include <string.h>
#include <vector>
#include <map>

#include <Newton.h>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Gek;

struct Header
{
    struct Material
    {
        wchar_t name[64] = L"";
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 0;
    uint16_t version = 6;

    Shapes::AlignedBox boundingBox;

    uint32_t partCount;
};

struct Part
{
    std::vector<uint16_t> indexList;
    std::vector<Math::Float3> vertexPositionList;
    std::vector<Math::Float2> vertexTexCoordList;
    std::vector<Math::Float3> vertexTangentList;
    std::vector<Math::Float3> vertexBiTangentList;
    std::vector<Math::Float3> vertexNormalList;
};

struct Parameters
{
    float feetPerUnit = 1.0f;
};

void getSceneParts(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::unordered_map<FileSystem::Path, std::vector<Part>> &scenePartMap, Shapes::AlignedBox &boundingBox)
{
    if (node == nullptr)
    {
        throw std::exception("Invalid scene node");
    }

    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            throw std::exception("Invalid mesh list");
        }

        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                throw std::exception("Invalid mesh index");
            }

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                if (mesh->mFaces == nullptr)
                {
                    throw std::exception("Invalid mesh face list");
                }

                if (mesh->mVertices == nullptr)
                {
                    throw std::exception("Invalid mesh vertex list");
                }

                if (mesh->mTextureCoords[0] == nullptr)
                {
                    throw std::exception("Invalid mesh texture coordinate list");
                }

                if (mesh->mTangents == nullptr)
                {
                    throw std::exception("Invalid mesh tangent list");
                }

                if (mesh->mBitangents == nullptr)
                {
                    throw std::exception("Invalid mesh bitangent list");
                }

                if (mesh->mNormals == nullptr)
                {
                    throw std::exception("Invalid mesh normal list");
                }

                Part part;
                part.indexList.resize(mesh->mNumFaces * 3);
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = mesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        throw std::exception("Non-triangular face encountered");
                    }

                    uint32_t edgeStartIndex = (faceIndex * 3);
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; edgeIndex++)
                    {
                        part.indexList[edgeStartIndex + edgeIndex] = face.mIndices[edgeIndex];
                    }
                }

                part.vertexPositionList.resize(mesh->mNumVertices);
                part.vertexTexCoordList.resize(mesh->mNumVertices);
                part.vertexTangentList.resize(mesh->mNumVertices);
                part.vertexBiTangentList.resize(mesh->mNumVertices);
                part.vertexNormalList.resize(mesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                {
                    part.vertexPositionList[vertexIndex].set(
                        (mesh->mVertices[vertexIndex].x * parameters.feetPerUnit),
                        (mesh->mVertices[vertexIndex].y * parameters.feetPerUnit),
                        (mesh->mVertices[vertexIndex].z * parameters.feetPerUnit));
                    boundingBox.extend(part.vertexPositionList[vertexIndex]);

                    part.vertexTexCoordList[vertexIndex].set(
                        mesh->mTextureCoords[0][vertexIndex].x,
                        mesh->mTextureCoords[0][vertexIndex].y);

                    part.vertexTangentList[vertexIndex].set(
                        mesh->mTangents[vertexIndex].x,
                        mesh->mTangents[vertexIndex].y,
                        mesh->mTangents[vertexIndex].z);

                    part.vertexBiTangentList[vertexIndex].set(
                        mesh->mBitangents[vertexIndex].x,
                        mesh->mBitangents[vertexIndex].y,
                        mesh->mBitangents[vertexIndex].z);

                    part.vertexNormalList[vertexIndex].set(
                        mesh->mNormals[vertexIndex].x,
                        mesh->mNormals[vertexIndex].y,
                        mesh->mNormals[vertexIndex].z);
                }

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                FileSystem::Path materialPath(sceneDiffuseMaterial.C_Str());
                scenePartMap[materialPath].push_back(std::move(part));
            }
        }
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            throw std::exception("Invalid child list");
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            getSceneParts(parameters, scene, node->mChildren[childIndex], scenePartMap, boundingBox);
        }
    }
}

void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fwrite(buffer, 1, size, file);
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    try
    {
        printf("GEK Part Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
        Parameters parameters;
        bool flipCoords = false;
        bool flipWinding = false;
        float smoothingAngle = 80.0f;
        for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            if (arguments.empty())
            {
                throw std::exception("No arguments specified for command line parameter");
            }

            if (arguments[0].compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-flipCoords") == 0)
            {
                flipCoords = true;
            }
            else if (arguments[0].compareNoCase(L"-flipWinding") == 0)
            {
                flipWinding = true;
            }
            else if (arguments[0].compareNoCase(L"-smoothAngle") == 0)
            {
                if (arguments.size() != 2)
                {
                    throw std::exception("Missing parameters for smoothAngle");
                }

                smoothingAngle = arguments[1];
            }
            else if (arguments[0].compareNoCase(L"-unitsInFoot") == 0)
            {
                if (arguments.size() != 2)
                {
                    throw std::exception("Missing parameters for unitsInFoot");
                }

                parameters.feetPerUnit = (1.0f / (float)arguments[1]);
            }
        }

        aiLogStream logStream;
        logStream.callback = [](char const *message, char *user) -> void
        {
            printf("Assimp: %s", message);
        };

        logStream.user = nullptr;
        aiAttachLogStream(&logStream);

        int notRequiredComponents =
            aiComponent_NORMALS |
            aiComponent_TANGENTS_AND_BITANGENTS |
            aiComponent_COLORS |
            aiComponent_BONEWEIGHTS |
            aiComponent_ANIMATIONS |
            aiComponent_LIGHTS |
            aiComponent_CAMERAS |
            0;

        unsigned int importFlags =
            (flipWinding ? aiProcess_FlipWindingOrder : 0) |
            (flipCoords ? aiProcess_FlipUVs : 0) |
            aiProcess_OptimizeMeshes |
            aiProcess_RemoveComponent |
            aiProcess_SplitLargeMeshes |
            aiProcess_PreTransformVertices |
            aiProcess_Triangulate |
            aiProcess_SortByPType |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_FindDegenerates |
            aiProcess_FindInstances |
            aiProcess_GenUVCoords |
            aiProcess_TransformUVCoords |
            0;

        unsigned int postProcessFlags =
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeGraph |
            0;

        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        auto scene = aiImportFileExWithProperties(StringUTF8(fileNameInput), importFlags, nullptr, propertyStore);
        if (scene == nullptr)
        {
            throw std::exception("Unable to load scene with Assimp");
        }

        scene = aiApplyPostProcessing(scene, postProcessFlags);
        if (scene == nullptr)
        {
            throw std::exception("Unable to apply post processing with Assimp");
        }

        if (!scene->HasMeshes())
        {
            throw std::exception("Scene has no meshes");
        }

        if (!scene->HasMaterials())
        {
            throw std::exception("Exporting to model requires materials in scene");
        }

        Shapes::AlignedBox boundingBox;
        std::unordered_map<FileSystem::Path, std::vector<Part>> scenePartMap;
        getSceneParts(parameters, scene, scene->mRootNode, scenePartMap, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, L"Data"));

        String texturesPath(FileSystem::GetFileName(dataPath, L"Textures"));
        texturesPath.toLower();

        String materialsPath(FileSystem::GetFileName(dataPath, L"Materials"));
        materialsPath.toLower();

        std::map<FileSystem::Path, String> pathToAlbedoMap;
        std::function<bool(const FileSystem::Path &)> findMaterials;
        findMaterials = [&](const FileSystem::Path &filePath) -> bool
        {
            if (filePath.isDirectory())
            {
                FileSystem::Find(filePath, findMaterials);
            }
            else if (filePath.isFile())
            {
                try
                {
                    const JSON::Object materialNode = JSON::Load(filePath);
                    auto &shaderNode = materialNode[L"shader"];
                    auto &passesNode = shaderNode[L"passes"];
                    auto &solidNode = passesNode[L"solid"];
                    auto &dataNode = solidNode[L"data"];
                    auto &albedoNode = dataNode[L"albedo"];
                    if (albedoNode.is_object())
                    {
                        if (albedoNode.has_member(L"file"))
                        {
                            String materialName(filePath.withoutExtension());
                            materialName = materialName.subString(materialsPath.size() + 1);
                            materialName.toLower();

                            FileSystem::Path albedoPath(albedoNode[L"file"].as_string());
                            pathToAlbedoMap[albedoPath] = materialName;

                            //printf("Material %S with %S albedo\r\n", materialName.c_str(), albedoPath.c_str());
                        }
                    }
                }
                catch (...)
                {
                };
            }

            return true;
        };

        auto engineIndex = texturesPath.find(L"gek engine");
        if (engineIndex != String::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.subString(engineIndex);
        }

        FileSystem::Find(materialsPath, findMaterials);
        std::unordered_map<FileSystem::Path, std::vector<Part>> albedoPartMap;
        for (auto &modelAlbedo : scenePartMap)
        {
            String albedoName(modelAlbedo.first.withoutExtension());
            albedoName.toLower();

            if (albedoName.find(L"textures\\") == 0)
            {
                albedoName = albedoName.subString(9);
            }
            else if (albedoName.find(L"..\\textures\\") == 0)
            {
                albedoName = albedoName.subString(12);
            }
            else
            {
                auto texturesIndex = albedoName.find(texturesPath);
                if (texturesIndex != String::npos)
                {
                    albedoName = albedoName.subString(texturesIndex + texturesPath.length() + 1);
                }
            }

            //printf("FileName: %S\r\n", modelAlbedo.first.c_str());
            //printf("Albedo: %S\r\n", albedoName.c_str());

            auto materialAlebedoSearch = pathToAlbedoMap.find(albedoName);
            if (materialAlebedoSearch == std::end(pathToAlbedoMap))
            {
                printf("! Unable to find material for albedo: %S\r\n", albedoName.c_str());
            }
            else
            {
                albedoPartMap[materialAlebedoSearch->second] = modelAlbedo.second;
                //printf("Remap: %S: %S\r\n", albedoName.c_str(), materialAlebedoSearch->second.c_str());
            }
        }

        std::unordered_map<FileSystem::Path, Part> materialPartMap;
        for (auto &multiMaterial : albedoPartMap)
        {
            Part &material = materialPartMap[multiMaterial.first];
            for (auto &instance : multiMaterial.second)
            {
                for (auto &index : instance.indexList)
                {
                    material.indexList.push_back(uint16_t(index + material.vertexPositionList.size()));
                }

                material.vertexPositionList.insert(std::end(material.vertexPositionList), std::begin(instance.vertexPositionList), std::end(instance.vertexPositionList));
                material.vertexTexCoordList.insert(std::end(material.vertexTexCoordList), std::begin(instance.vertexTexCoordList), std::end(instance.vertexTexCoordList));
                material.vertexTangentList.insert(std::end(material.vertexTangentList), std::begin(instance.vertexTangentList), std::end(instance.vertexTangentList));
                material.vertexBiTangentList.insert(std::end(material.vertexBiTangentList), std::begin(instance.vertexBiTangentList), std::end(instance.vertexBiTangentList));
                material.vertexNormalList.insert(std::end(material.vertexNormalList), std::begin(instance.vertexNormalList), std::end(instance.vertexNormalList));
            }
        }

        if (materialPartMap.empty())
        {
            throw std::exception("No valid material models found");
        }

        printf("> Num. Parts: %d\r\n", materialPartMap.size());
        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

        FILE *file = nullptr;
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.partCount = materialPartMap.size();
        header.boundingBox = boundingBox;
        fwrite(&header, sizeof(Header), 1, file);
        for (auto &material : materialPartMap)
        {
            printf("-  %S\r\n", material.first.c_str());
            printf("    %d vertices\r\n", material.second.vertexPositionList.size());
            printf("    %d indices\r\n", material.second.indexList.size());

            Header::Material materialHeader;
            std::wcsncpy(materialHeader.name, material.first, 63);
            materialHeader.vertexCount = material.second.vertexPositionList.size();
            materialHeader.indexCount = material.second.indexList.size();
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        for (auto &material : materialPartMap)
        {
            fwrite(material.second.indexList.data(), sizeof(uint16_t), material.second.indexList.size(), file);
            fwrite(material.second.vertexPositionList.data(), sizeof(Math::Float3), material.second.vertexPositionList.size(), file);
            fwrite(material.second.vertexTexCoordList.data(), sizeof(Math::Float2), material.second.vertexTexCoordList.size(), file);
            fwrite(material.second.vertexTangentList.data(), sizeof(Math::Float3), material.second.vertexTangentList.size(), file);
            fwrite(material.second.vertexBiTangentList.data(), sizeof(Math::Float3), material.second.vertexBiTangentList.size(), file);
            fwrite(material.second.vertexNormalList.data(), sizeof(Math::Float3), material.second.vertexNormalList.size(), file);
        }

        fclose(file);
    }
    catch (const std::exception &exception)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf(StringUTF8::Format("Caught: %v\r\nType: %v\r\n", exception.what(), typeid(exception).name()));
    }
    catch (...)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf("Caught: Non-standard exception\r\n");
    };

    printf("\r\n");
    return 0;
}
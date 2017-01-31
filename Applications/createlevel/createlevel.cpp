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

    uint32_t instanceCount;
    uint32_t partIndexCount;
    uint32_t partCount;
};

struct Instance
{
    Math::Float4x4 transform;
    uint32_t partIndexStart;
    uint32_t partIndexCount;
};

struct Part
{
    FileSystem::Path albedoPath;
    Shapes::AlignedBox boundingBox;
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

void getSceneParts(const Parameters &parameters, const aiScene *scene, std::vector<Part> &scenePartList)
{
    if (scene->mNumMeshes == 0)
    {
        throw std::exception("Invalid mesh list");
    }

    if (scene->mMeshes == nullptr)
    {
        throw std::exception("Invalid mesh list");
    }

    scenePartList.resize(scene->mNumMeshes);
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh *mesh = scene->mMeshes[meshIndex];
        if (mesh->mNumFaces == 0)
        {
            throw std::exception("Invalid mesh face list");
        }

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

        Part &part = scenePartList[meshIndex];
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
            part.boundingBox.extend(part.vertexPositionList[vertexIndex]);

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
        part.albedoPath = sceneDiffuseMaterial.C_Str();
    }
}

void getSceneInstances(const Parameters &parameters, const aiScene *scene, const aiNode *node, const Math::Float4x4 &parentTransform, std::vector<Instance> &instanceList, std::vector<uint32_t> &partIndexList)
{
    if (node == nullptr)
    {
        throw std::exception("Invalid scene node");
    }

    auto localTransform(*(Math::Float4x4 *)&node->mTransformation);
    auto absoluteTransform(parentTransform * localTransform);
    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            throw std::exception("Invalid mesh list");
        }

        Instance instance;
        instance.transform = absoluteTransform;
        instance.partIndexStart = partIndexList.size();
        instance.partIndexCount = node->mNumMeshes;
        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                throw std::exception("Invalid mesh index");
            }

            partIndexList.push_back(nodeMeshIndex);
        }

        instanceList.push_back(instance);
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            throw std::exception("Invalid child list");
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            getSceneInstances(parameters, scene, node->mChildren[childIndex], absoluteTransform, instanceList, partIndexList);
        }
    }
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

        std::vector<Part> scenePartList;
        getSceneParts(parameters, scene, scenePartList);
        if (scenePartList.empty())
        {
            throw std::exception("No valid models found");
        }

        std::vector<Instance> instanceList;
        std::vector<uint32_t> partIndexList;
        getSceneInstances(parameters, scene, scene->mRootNode, Math::Float4x4::Identity, instanceList, partIndexList);
        if (instanceList.empty())
        {
            throw std::exception("No valid instances found");
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, L"Data"));

        String texturesPath(FileSystem::GetFileName(dataPath, L"Textures"));
        texturesPath.toLower();

        String materialsPath(FileSystem::GetFileName(dataPath, L"Materials"));
        materialsPath.toLower();

        std::map<FileSystem::Path, String> albedoToMaterialMap;
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
                            albedoToMaterialMap[albedoPath] = materialName;

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

        printf("> Num. Instances: %d\r\n", instanceList.size());
        printf("> Num. Part Indices: %d\r\n", partIndexList.size());
        printf("> Num. Parts: %d\r\n", scenePartList.size());

        FILE *file = nullptr;
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.instanceCount = instanceList.size();
        header.partIndexCount = partIndexList.size();
        header.partCount = scenePartList.size();
        fwrite(&header, sizeof(Header), 1, file);

        fwrite(instanceList.data(), sizeof(Instance), instanceList.size(), file);
        fwrite(partIndexList.data(), sizeof(uint32_t), partIndexList.size(), file);

        for (auto &part : scenePartList)
        {
            String albedoName(part.albedoPath.withoutExtension());
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

            Header::Material materialHeader;
            auto materialAlebedoSearch = albedoToMaterialMap.find(albedoName);
            if (materialAlebedoSearch != std::end(albedoToMaterialMap))
            {
                std::wcsncpy(materialHeader.name, materialAlebedoSearch->second.c_str(), 63);
            }

            printf("-    Material: %S\r\n", materialHeader.name);
            printf("       Num. Vertices: %d\r\n", part.vertexPositionList.size());
            printf("       Num. Indices: %d\r\n", part.indexList.size());

            materialHeader.vertexCount = part.vertexPositionList.size();
            materialHeader.indexCount = part.indexList.size();
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        for (auto &part : scenePartList)
        {
            fwrite(part.indexList.data(), sizeof(uint16_t), part.indexList.size(), file);
            fwrite(part.vertexPositionList.data(), sizeof(Math::Float3), part.vertexPositionList.size(), file);
            fwrite(part.vertexTexCoordList.data(), sizeof(Math::Float2), part.vertexTexCoordList.size(), file);
            fwrite(part.vertexTangentList.data(), sizeof(Math::Float3), part.vertexTangentList.size(), file);
            fwrite(part.vertexBiTangentList.data(), sizeof(Math::Float3), part.vertexBiTangentList.size(), file);
            fwrite(part.vertexNormalList.data(), sizeof(Math::Float3), part.vertexNormalList.size(), file);
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
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
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
        char name[64] = "";
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
    std::string forceMaterial;
};

bool getSceneParts(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::unordered_map<std::string, std::vector<Part>> &scenePartMap, Shapes::AlignedBox &boundingBox)
{
    if (node == nullptr)
    {
        std::cerr << "Invalid scene node" << std::endl;
        return false;
    }

    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            std::cerr << "Invalid mesh list" << std::endl;
            return false;
        }

        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                std::cerr << "Invalid mesh index" << std::endl;
                return false;
            }

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                if (mesh->mFaces == nullptr)
                {
                    std::cerr << "Invalid mesh face list" << std::endl;
                    return false;
                }

                if (mesh->mVertices == nullptr)
                {
                    std::cerr << "Invalid mesh vertex list" << std::endl;
                    return false;
                }

                if (mesh->mTextureCoords[0] == nullptr)
                {
                    std::cerr << "Invalid mesh texture coordinate list" << std::endl;
                    return false;
                }

                if (mesh->mTangents == nullptr)
                {
                    std::cerr << "Invalid mesh tangent list" << std::endl;
                    return false;
                }

                if (mesh->mBitangents == nullptr)
                {
                    std::cerr << "Invalid mesh bitangent list" << std::endl;
                    return false;
                }

                if (mesh->mNormals == nullptr)
                {
                    std::cerr << "Invalid mesh normal list" << std::endl;
                    return false;
                }

                Part part;
                part.indexList.resize(mesh->mNumFaces * 3);
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = mesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        std::cerr << "Non-triangular face encountered" << std::endl;
                        return false;
                    }

                    uint32_t edgeStartIndex = (faceIndex * 3);
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
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

                if (parameters.forceMaterial.empty())
                {
                    aiString sceneDiffuseMaterial;
                    const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                    sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                    std::string materialPath = sceneDiffuseMaterial.C_Str();
                    scenePartMap[materialPath].push_back(std::move(part));
                }
                else
                {
                    scenePartMap[parameters.forceMaterial].push_back(std::move(part));
                }
            }
        }
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            std::cerr << "Invalid child list" << std::endl;
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            if (!getSceneParts(parameters, scene, node->mChildren[childIndex], scenePartMap, boundingBox))
            {
                return false;
            }
        }
    }

    return true;
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    std::cout << "GEK Part Converter" << std::endl;

    FileSystem::Path fileNameInput;
    FileSystem::Path fileNameOutput;
    Parameters parameters;
    bool flipCoords = false;
    bool flipWinding = false;
    float smoothingAngle = 80.0f;
    for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
    {
		std::string argument(String::Narrow(argumentList[argumentIndex]));
		std::vector<std::string> arguments(String::Split(String::GetLower(argument), ':'));
        if (arguments.empty())
        {
            std::cerr << "No arguments specified for command line parameter" << std::endl;
            return -__LINE__;
        }

        if (arguments[0] == "-input" && ++argumentIndex < argumentCount)
        {
			fileNameInput = argumentList[argumentIndex];
        }
        else if (arguments[0] == "-output" && ++argumentIndex < argumentCount)
        {
			fileNameOutput = argumentList[argumentIndex];
        }
        else if (arguments[0] == "-flipcoords")
        {
            flipCoords = true;
        }
        else if (arguments[0] == "-flipwinding")
        {
            flipWinding = true;
        }
        else if (arguments[0] == "-smoothangle")
        {
            if (arguments.size() != 2)
            {
                std::cerr << "Missing parameters for smoothAngle" << std::endl;
                return -__LINE__;
            }

			smoothingAngle = String::Convert(arguments[1], 80.0f);
        }
        else if (arguments[0] == "-unitsinfoot")
        {
            if (arguments.size() != 2)
            {
                std::cerr << "Missing parameters for unitsInFoot" << std::endl;
                return -__LINE__;
            }

			parameters.feetPerUnit = (1.0f / String::Convert(arguments[1], 1.0f));
        }
        else if (arguments[0] == "-forcematerial" && ++argumentIndex < argumentCount)
        {
            parameters.forceMaterial = String::Narrow(argumentList[argumentIndex]);
        }
    }

    aiLogStream logStream;
    logStream.callback = [](char const *message, char *user) -> void
    {
		std::cerr << "Assimp: " << message;
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
        0;

    unsigned int textureProcessFlags =
        aiProcess_GenUVCoords |
        //aiProcess_GenUVCoords_Sphere |
        aiProcess_TransformUVCoords |
        0;

    unsigned int tangentProcessFlags =
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
    auto scene = aiImportFileExWithProperties(fileNameInput.u8string().c_str(), importFlags, nullptr, propertyStore);
    if (scene == nullptr)
    {
        std::cerr << "Unable to load scene with Assimp" << std::endl;
        return -__LINE__;
    }

    scene = aiApplyPostProcessing(scene, textureProcessFlags);
    if (scene == nullptr)
    {
        std::cerr << "Unable to apply texture post processing with Assimp" << std::endl;
        return -__LINE__;
    }

    scene = aiApplyPostProcessing(scene, tangentProcessFlags);
    if (scene == nullptr)
    {
        std::cerr << "Unable to apply tangent post processing with Assimp" << std::endl;
        return -__LINE__;
    }

    if (!scene->HasMeshes())
    {
        std::cerr << "Scene has no meshes" << std::endl;
        return -__LINE__;
    }

    if (!scene->HasMaterials())
    {
        std::cerr << "Exporting to model requires materials in scene" << std::endl;
        return -__LINE__;
    }

    Shapes::AlignedBox boundingBox;
    std::unordered_map<std::string, std::vector<Part>> scenePartMap;
    if (!getSceneParts(parameters, scene, scene->mRootNode, scenePartMap, boundingBox))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(scene);

    auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
    auto dataPath(FileSystem::GetFileName(rootPath, "Data"));

	std::string texturesPath(String::GetLower(FileSystem::GetFileName(dataPath, "Textures").u8string()));
    auto materialsPath(FileSystem::GetFileName(dataPath, "Materials").u8string());

	std::map<std::string, std::string> albedoToMaterialMap;
    std::function<bool(FileSystem::Path const &)> findMaterials;
    findMaterials = [&](FileSystem::Path const &filePath) -> bool
    {
        if (filePath.isDirectory())
        {
            FileSystem::Find(filePath, findMaterials);
        }
        else if (filePath.isFile())
        {
            const JSON::Object materialNode = JSON::Load(filePath);
            auto &shaderNode = materialNode["shader"];
            auto &passesNode = shaderNode["passes"];
            auto &solidNode = passesNode["solid"];
            auto &dataNode = solidNode["data"];
            auto &albedoNode = dataNode["albedo"];
            auto albedoPath = albedoNode["file"].string_value();
            if (!albedoPath.empty())
            {
                std::string materialName(String::GetLower(filePath.withoutExtension().u8string().substr(materialsPath.size() + 1)));
                albedoToMaterialMap[albedoPath] = materialName;
            }
        }

        return true;
    };

    auto engineIndex = texturesPath.find("gek engine");
    if (engineIndex != std::string::npos)
    {
        // skip hard drive location, jump to known engine structure
        texturesPath = texturesPath.substr(engineIndex);
    }

    FileSystem::Find(materialsPath, findMaterials);
    if (albedoToMaterialMap.empty())
    {
        std::cerr << "Unable to locate any materials" << std::endl;
        return -__LINE__;
    }

    std::unordered_map<std::string, std::vector<Part>> albedoPartMap;
    for (const auto &modelAlbedo : scenePartMap)
    {
		std::string albedoName(String::GetLower(FileSystem::Path(modelAlbedo.first).withoutExtension().u8string()));
        std::cout << "Found Albedo: " << albedoName << std::endl;
        if (albedoName.find("textures\\") == 0)
        {
            albedoName = albedoName.substr(9);
        }
        else if (albedoName.find("..\\textures\\") == 0)
        {
            albedoName = albedoName.substr(12);
        }
        else if (albedoName.find("..\\..\\textures\\") == 0)
        {
            albedoName = albedoName.substr(15);
        }
        else
        {
            auto texturesIndex = albedoName.find(texturesPath);
            if (texturesIndex != std::string::npos)
            {
                albedoName = albedoName.substr(texturesIndex + texturesPath.length() + 1);
            }
        }

        auto materialAlebedoSearch = albedoToMaterialMap.find(albedoName);
        if (materialAlebedoSearch == std::end(albedoToMaterialMap))
        {
            std::cerr << "! Unable to find material for albedo: " << albedoName.c_str() << std::endl;
        }
        else
        {
            albedoPartMap[materialAlebedoSearch->second] = modelAlbedo.second;
        }
    }

    std::unordered_map<std::string, Part> materialPartMap;
    for (const auto &multiMaterial : albedoPartMap)
    {
        Part &material = materialPartMap[multiMaterial.first];
        for (const auto &instance : multiMaterial.second)
        {
            for (const auto &index : instance.indexList)
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
        std::cerr << "No valid material models found" << std::endl;
        return -__LINE__;
    }

	std::cout << "> Num. Parts: " << materialPartMap.size() << std::endl;
	std::cout << "< Size: Min(" << boundingBox.minimum.x << ", " << boundingBox.minimum.y << ", " << boundingBox.minimum.z << ")" << std::endl;
	std::cout << "<       Max(" << boundingBox.maximum.x << ", " << boundingBox.maximum.y << ", " << boundingBox.maximum.z << ")" << std::endl;

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput.c_str(), L"wb");
    if (file == nullptr)
    {
        std::cerr << "Unable to create output file" << std::endl;
        return -__LINE__;
    }

    Header header;
    header.partCount = materialPartMap.size();
    header.boundingBox = boundingBox;
    fwrite(&header, sizeof(Header), 1, file);
    for (const auto &material : materialPartMap)
    {
		std::string name = material.first;
		std::cout << "-    Material: " << name << std::endl;
        std::cout << "       Num. Vertices: " << material.second.vertexPositionList.size() << std::endl;
        std::cout << "       Num. Indices: " << material.second.indexList.size() << std::endl;

        Header::Material materialHeader;
        std::strncpy(materialHeader.name, name.c_str(), 63);
        materialHeader.vertexCount = material.second.vertexPositionList.size();
        materialHeader.indexCount = material.second.indexList.size();
        fwrite(&materialHeader, sizeof(Header::Material), 1, file);
    }

    for (const auto &material : materialPartMap)
    {
        fwrite(material.second.indexList.data(), sizeof(uint16_t), material.second.indexList.size(), file);
        fwrite(material.second.vertexPositionList.data(), sizeof(Math::Float3), material.second.vertexPositionList.size(), file);
        fwrite(material.second.vertexTexCoordList.data(), sizeof(Math::Float2), material.second.vertexTexCoordList.size(), file);
        fwrite(material.second.vertexTangentList.data(), sizeof(Math::Float3), material.second.vertexTangentList.size(), file);
        fwrite(material.second.vertexBiTangentList.data(), sizeof(Math::Float3), material.second.vertexBiTangentList.size(), file);
        fwrite(material.second.vertexNormalList.data(), sizeof(Math::Float3), material.second.vertexNormalList.size(), file);
    }

    fclose(file);

    return 0;
}
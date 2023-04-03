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
#include <vector>
#include <map>
#include <set>

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
        char name[64] = "";
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 2;
    uint16_t version = 2;
    uint32_t newtonVersion = NewtonWorldGetVersion();

    uint32_t materialCount = 0;
};

struct Mesh
{
    struct Face
    {
        uint16_t data[3];
        uint16_t& operator [] (size_t index)
        {
            return data[index];
        }

        const uint16_t& operator [] (size_t index) const
        {
            return data[index];
        }
    };

    std::string material;
    std::vector<Math::Float3> pointList;
    std::vector<Face> faceList;
};

struct Model
{
    Shapes::AlignedBox boundingBox;
    std::vector<Mesh> meshList;
};

struct Parameters
{
    std::string sourceName;
    std::string targetName;
    float feetPerUnit = 1.0f;
    bool flipWinding = false;
};

bool GetModels(Parameters const& parameters, aiScene const* inputScene, aiNode const* inputNode, aiMatrix4x4 const& parentTransform, Model& model, std::function<std::string(const std::string&, const std::string&)> findMaterialForMesh)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << "Invalid scene node";
        return false;
    }

    aiMatrix4x4 transform(parentTransform * inputNode->mTransformation);
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid mesh list";
            return false;
        }

        std::string name = inputNode->mName.C_Str();
        LockedWrite{ std::cout } << "Found Assimp Model: " << name;
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                LockedWrite{ std::cerr } << "Invalid mesh index";
                continue;
            }

            const aiMesh* inputMesh = inputScene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh face list";
                    continue;
                }

                if (inputMesh->mVertices == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh vertex list";
                    continue;
                }

                Mesh mesh;

                aiString sceneDiffuseMaterial;
                const aiMaterial* sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string diffuseName = sceneDiffuseMaterial.C_Str();
                mesh.material = findMaterialForMesh(parameters.sourceName, diffuseName);
                if (mesh.material.empty())
                {
                    LockedWrite{ std::cerr } << "Unable to find material for mesh " << inputMesh->mName.C_Str();
                    continue;
                }

                mesh.pointList.resize(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertex = inputMesh->mVertices[vertexIndex];
                    aiTransformVecByMatrix4(&vertex, &transform);
                    mesh.pointList[vertexIndex].set(vertex.x, vertex.y, vertex.z);
                    mesh.pointList[vertexIndex] *= parameters.feetPerUnit;
                    model.boundingBox.extend(mesh.pointList[vertexIndex]);
                }

                mesh.faceList.reserve(inputMesh->mNumFaces);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace& face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        LockedWrite{ std::cerr } << "Skipping non-triangular face, face: " << faceIndex << ": " << face.mNumIndices << " indices";
                        continue;
                    }

                    Mesh::Face meshFace;
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        meshFace[edgeIndex] = face.mIndices[edgeIndex];
                    }

                    mesh.faceList.push_back(meshFace);
                }

                model.meshList.push_back(mesh);
            }
        }
    }

    if (inputNode->mNumChildren > 0)
    {
        if (inputNode->mChildren == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid child list";
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            if (!GetModels(parameters, inputScene, inputNode->mChildren[childIndex], transform, model, findMaterialForMesh))
            {
                return false;
            }
        }
    }

    return true;
}

void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fwrite(buffer, 1, size, file);
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{ std::cout } << "GEK Tree Converter";

    Parameters parameters;
    for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
    {
        std::string argument(String::Narrow(argumentList[argumentIndex]));
        std::vector<std::string> arguments(String::Split(String::GetLower(argument), ':'));
        if (arguments.empty())
        {
            LockedWrite{ std::cerr } << "No arguments specified for command line parameter";
            return -__LINE__;
        }

        if (arguments[0] == "-source" && ++argumentIndex < argumentCount)
        {
            parameters.sourceName = String::Narrow(argumentList[argumentIndex]);
        }
        else if (arguments[0] == "-target" && ++argumentIndex < argumentCount)
        {
            parameters.targetName = String::Narrow(argumentList[argumentIndex]);
        }
        else if (arguments[0] == "-flipwinding")
        {
            parameters.flipWinding = true;
        }
        else if (arguments[0] == "-unitsinfoot")
        {
            if (arguments.size() != 2)
            {
                LockedWrite{ std::cerr } << "Missing parameters for unitsInFoot";
                return -__LINE__;
            }

            parameters.feetPerUnit = (1.0f / String::Convert(arguments[1], 1.0f));
        }
    }

    aiLogStream logStream;
    logStream.callback = [](char const* message, char* user) -> void
    {
        std::string trimmedMessage(message);
        trimmedMessage = trimmedMessage.substr(0, trimmedMessage.size() - 1);
        LockedWrite{ std::cerr } << "Assimp: " << trimmedMessage;
    };

    logStream.user = nullptr;
    aiAttachLogStream(&logStream);

    int notRequiredComponents =
        //aiComponent_NORMALS |
        //aiComponent_TANGENTS_AND_BITANGENTS |
        aiComponent_COLORS |
        aiComponent_BONEWEIGHTS |
        aiComponent_ANIMATIONS |
        aiComponent_LIGHTS |
        aiComponent_CAMERAS |
        0;

    static const unsigned int importFlags =
        (parameters.flipWinding ? aiProcess_FlipWindingOrder : 0) |
        aiProcess_RemoveComponent |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        aiProcess_ValidateDataStructure;

    aiPropertyStore* propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);

    FileSystem::Path dataPath;
    wchar_t gekDataPath[MAX_PATH + 1] = L"\0";
    if (GetEnvironmentVariable(L"gek_data_path", gekDataPath, MAX_PATH) > 0)
    {
        dataPath = String::Narrow(gekDataPath);
    }
    else
    {
        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        dataPath = rootPath / "Data"sv;
    }

    auto sourcePath(dataPath / "physics"sv / parameters.sourceName);
    LockedWrite{ std::cout } << "Loading: " << sourcePath.getString();
    auto inputScene = aiImportFileExWithProperties(sourcePath.getString().data(), importFlags, nullptr, propertyStore);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
        return -__LINE__;
    }

    static const unsigned int postProcessSteps[] =
    {
        aiProcess_Triangulate |
        //aiProcess_SplitLargeMeshes |
        0,

        aiProcess_JoinIdenticalVertices |
        aiProcess_FindInvalidData |
        0,

        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph,
        0,
    };

    for (auto postProcessFlags : postProcessSteps)
    {
        inputScene = aiApplyPostProcessing(inputScene, postProcessFlags);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to apply post processing with Assimp";
            return -__LINE__;
        }
    }

    if (!inputScene->HasMeshes())
    {
        LockedWrite{ std::cerr } << "Scene has no meshes";
        return -__LINE__;
    }

    if (!inputScene->HasMaterials())
    {
        LockedWrite{ std::cerr } << "Exporting to model requires materials in scene";
        return -__LINE__;
    }

    std::string texturesPath(String::GetLower((dataPath / "Textures"sv).getString()));
    auto materialsPath((dataPath / "Materials"sv).getString());

    std::map<std::string, std::string> diffuseToMaterialMap;
    std::function<bool(FileSystem::Path const&)> findMaterials;
    findMaterials = [&](FileSystem::Path const& filePath) -> bool
    {
        if (filePath.isDirectory())
        {
            filePath.findFiles(findMaterials);
        }
        else if (filePath.isFile())
        {
            JSON materialNode;
            materialNode.load(filePath);
            auto& shaderNode = materialNode.getMember("shader");
            auto& dataNode = shaderNode.getMember("data");
            auto& albedoNode = dataNode.getMember("albedo");
            std::string albedoPath(albedoNode.getMember("file").convert(String::Empty));
            std::string materialName(String::GetLower(filePath.withoutExtension().getString().substr(materialsPath.size() + 1)));
            if (!albedoPath.empty() && !materialName.empty())
            {
                diffuseToMaterialMap[albedoPath] = materialName;
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

    FileSystem::Path(materialsPath).findFiles(findMaterials);
    if (diffuseToMaterialMap.empty())
    {
        LockedWrite{ std::cerr } << "Unable to locate any materials";
        return -__LINE__;
    }

    auto findMaterialForMesh = [&](const FileSystem::Path& sourceName, const std::string& diffuseName) -> std::string
    {
        FileSystem::Path diffusePath(diffuseName);
        std::string albedoName(String::GetLower(diffusePath.withoutExtension().getString()));
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

        if (albedoName.ends_with("_basecolor"))
        {
            albedoName = albedoName.substr(0, albedoName.size() - 10);
        }

        auto materialAlebedoSearch = diffuseToMaterialMap.find(albedoName);
        if (materialAlebedoSearch != std::end(diffuseToMaterialMap))
        {
            return materialAlebedoSearch->second;
        }

        auto sourceAlbedoName = (sourceName.withoutExtension() / albedoName).getString();
        materialAlebedoSearch = diffuseToMaterialMap.find(sourceAlbedoName);
        if (materialAlebedoSearch != std::end(diffuseToMaterialMap))
        {
            return materialAlebedoSearch->second;
        }

        auto doesValueExist = [&](const std::string& value) -> bool
        {
            for (auto& iterator : diffuseToMaterialMap)
            {
                if (iterator.second == value)
                {
                    return true;
                }
            }

            return false;
        };

        if (doesValueExist(sourceAlbedoName))
        {
            return sourceAlbedoName;
        }

        LockedWrite{ std::cerr } << "   ! Unable to find material for albedo: " << albedoName;
        return "";
    };

    Model model;
    aiMatrix4x4 identity;
    if (!GetModels(parameters, inputScene, inputScene->mRootNode, identity, model, findMaterialForMesh))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(inputScene);

    LockedWrite{ std::cout } << "> Num. Meshes: " << model.meshList.size();
    LockedWrite{ std::cout } << "< Size: Minimum[" << model.boundingBox.minimum.x << ", " << model.boundingBox.minimum.y << ", " << model.boundingBox.minimum.z << "]";
    LockedWrite{ std::cout } << "< Size: Maximum[" << model.boundingBox.maximum.x << ", " << model.boundingBox.maximum.y << ", " << model.boundingBox.maximum.z << "]";

    NewtonWorld *newtonWorld = NewtonCreate();
    NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
    if (newtonCollision == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to create tree collision object";
    }

    std::set<std::string> materialList;
    for (auto &mesh : model.meshList)
    {
        materialList.insert(mesh.material);
    }

    NewtonTreeCollisionBeginBuild(newtonCollision);
    for (auto const &mesh : model.meshList)
    {
		LockedWrite{ std::cout } << "-   " << mesh.material;
        LockedWrite{ std::cout } << "    " << mesh.pointList.size() << "  vertices";
        LockedWrite{ std::cout } << "    " << mesh.faceList.size() << " faces";

        auto materialSearch = materialList.find(mesh.material);
        auto materialIndex = std::distance(std::begin(materialList), materialSearch);

        const auto &faceList = mesh.faceList;
        const auto &vertexList = mesh.pointList;
        for (uint32_t index = 0; index < faceList.size(); index += 3)
        {
            const auto& face = faceList[index];
            Math::Float3 faceData[3] =
            {
                vertexList[face.data[0]],
                vertexList[face.data[1]],
                vertexList[face.data[2]],
            };

            NewtonTreeCollisionAddFace(newtonCollision, 3, faceData[0].data, sizeof(Math::Float3), materialIndex);
        }
    }

    NewtonTreeCollisionEndBuild(newtonCollision, 0);

    auto outputPath((dataPath / "physics"sv / parameters.targetName).withoutExtension().withExtension(".gek"));
    LockedWrite{ std::cout } << "Writing: " << outputPath.getString();
    outputPath.getParentPath().createChain();

    FILE *file = nullptr;
    _wfopen_s(&file, outputPath.getWideString().data(), L"wb");
    if (file == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to create output file";
    }

    Header header;
    header.materialCount = materialList.size();
    fwrite(&header, sizeof(Header), 1, file);
    for (auto const &material : materialList)
    {
        Header::Material materialHeader;
        std::strncpy(materialHeader.name, material.data(), 63);
        fwrite(&materialHeader, sizeof(Header::Material), 1, file);
    }

    NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
    fclose(file);

    NewtonDestroyCollision(newtonCollision);
    NewtonDestroy(newtonWorld);
    return 0;
}
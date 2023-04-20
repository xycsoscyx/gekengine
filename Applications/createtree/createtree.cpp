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

#include <argparse/argparse.hpp>
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

    struct Mesh
    {
        uint32_t materialIndex;
        uint32_t faceCount;
        uint32_t pointCount;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 2;
    uint16_t version = 3;

    uint32_t materialCount = 0;
    uint32_t meshCount = 0;
};

struct Mesh
{
    struct Face
    {
        int32_t data[3];
        int32_t& operator [] (size_t index)
        {
            return data[index];
        }

        const int32_t& operator [] (size_t index) const
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
    float feetPerUnit;
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

    argparse::ArgumentParser program("GEK Tree Converter", "1.0");

    program.add_argument("-i", "--input")
        .required()
        .help("input model");

    program.add_argument("-o", "--output")
        .required()
        .help("output model");

    program.add_argument("-u", "--unitsperfoot")
        .scan<'g', float>()
        .help("units per foot")
        .default_value(1.0f);

    program.add_description("Convert input model in to GEK Engine format.");
    program.add_epilog("Input model formats include anything supported by the Assimp library.");

    try
    {
        std::vector<std::string> arguments;
        for (int argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
        {
            arguments.push_back(String::Narrow(argumentList[argumentIndex]));
        }

        program.parse_args(arguments);
    }
    catch (const std::runtime_error& err)
    {
        LockedWrite{ std::cerr } << err.what() << std::endl;
        LockedWrite{ std::cerr } << program;
        return 1;
    }

    Parameters parameters;
    parameters.sourceName = program.get<std::string>("--input");
    parameters.targetName = program.get<std::string>("--output");
    parameters.feetPerUnit = (1.0f / program.get<float>("--unitsperfoot"));

    aiLogStream logStream;
    logStream.callback = [](char const* message, char* user) -> void
    {
        std::string trimmedMessage(message);
        trimmedMessage = trimmedMessage.substr(0, trimmedMessage.size() - 1);
        LockedWrite{ std::cerr } << "Assimp: " << trimmedMessage;
    };

    logStream.user = nullptr;
    aiAttachLogStream(&logStream);

    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto rootPath(pluginPath.getParentPath());
    auto cachePath(rootPath / "cache");
    SetCurrentDirectoryW(cachePath.getWideString().data());

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    ContextPtr context(Context::Create(nullptr));
    if (context)
    {
        context->setCachePath(cachePath);

        wchar_t gekDataPath[MAX_PATH + 1] = L"\0";
        if (GetEnvironmentVariable(L"gek_data_path", gekDataPath, MAX_PATH) > 0)
        {
            context->addDataPath(String::Narrow(gekDataPath));
        }

        context->addDataPath(rootPath / "data");
        context->addDataPath(rootPath.getString());

        auto filePath = context->findDataPath(FileSystem::CreatePath("physics", parameters.sourceName));


        int notRequiredComponents =
            aiComponent_NORMALS |
            aiComponent_TANGENTS_AND_BITANGENTS |
            aiComponent_COLORS |
            aiComponent_BONEWEIGHTS |
            aiComponent_ANIMATIONS |
            aiComponent_LIGHTS |
            aiComponent_CAMERAS |
            0;

        static const unsigned int importFlags =
            aiProcess_RemoveComponent |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_FindDegenerates |
            aiProcess_ValidateDataStructure |
            aiProcess_MakeLeftHanded |
            aiProcess_FlipWindingOrder |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            //aiProcess_OptimizeGraph,
            0;

        aiPropertyStore* propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        //aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        LockedWrite{ std::cout } << "Loading: " << filePath.getString();
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_Triangulate);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to apply post processing with Assimp";
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_SplitLargeMeshes);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to apply post processing with Assimp";
            return -__LINE__;
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

        auto texturesPath(context->findDataPath("textures").getString());
        auto engineIndex = texturesPath.find("gek engine");
        if (engineIndex != std::string::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.substr(engineIndex);
        }

        std::function<FileSystem::Path(const char*, FileSystem::Path const&)> removeRoot = [](const char* location, FileSystem::Path const& path) -> FileSystem::Path
        {
            auto parentPath = path.getParentPath();
            while (parentPath.isDirectory() && parentPath != path.getRootPath())
            {
                if (parentPath.getFileName() == location)
                {
                    return path.lexicallyRelative(parentPath);
                }
                else
                {
                    parentPath = parentPath.getParentPath();
                }
            };

            return path;
        };

        std::map<std::string, std::string> albedoToMaterialMap;
        std::function<bool(FileSystem::Path const&)> findMaterials;
        findMaterials = [&](FileSystem::Path const& filePath) -> bool
        {
            if (filePath.getExtension() != ".json")
            {
                return true;
            }

            JSON materialNode;
            materialNode.load(filePath);
            auto& shaderNode = materialNode.getMember("shader");
            auto& dataNode = shaderNode.getMember("data");
            auto& albedoNode = dataNode.getMember("albedo");
            auto albedoFile = albedoNode.getMember("file").convert(String::Empty);
            auto albedoPath = removeRoot("textures", context->findDataPath(albedoFile));

            LockedWrite{ std::cerr } << "Found material: " << filePath.getString() << ", with albedo: " << albedoFile;
            albedoToMaterialMap[String::GetLower(albedoFile)] = String::GetLower(removeRoot("materials", filePath).getString());
            return true;
        };

        context->findDataFiles("materials", findMaterials);
        if (albedoToMaterialMap.empty())
        {
            LockedWrite{ std::cerr } << "Unable to locate any materials";
            return -__LINE__;
        }

        auto findMaterialForMesh = [&](const FileSystem::Path& sourceName, std::string diffuseName) -> std::string
        {
            LockedWrite{ std::cout } << "> Searching for : " << diffuseName << ", " << (filePath / diffuseName).getString();

            FileSystem::Path albedoPath = FileSystem::GetCanonicalPath(filePath / diffuseName);
            if (!albedoPath.isFile())
            {
                albedoPath = FileSystem::Path(diffuseName).lexicallyRelative("textures");
                albedoPath = context->findDataPath(FileSystem::Path("textures") / filePath.withoutExtension().getFileName() / albedoPath);
            }

            if (albedoPath.isFile())
            {
                albedoPath = removeRoot("textures", albedoPath);
                auto albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.withoutExtension().getString()));
                if (albedoSearch == std::end(albedoToMaterialMap))
                {
                    albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.getString()));
                }

                if (albedoSearch != std::end(albedoToMaterialMap))
                {
                    LockedWrite{ std::cout } << "  Found material for albedo: " << albedoPath.getString() << " belongs to " << albedoSearch->second;
                    return albedoSearch->second;
                }
            }

            LockedWrite{ std::cerr } << "! Unable to find material for albedo: " << diffuseName << ", " << albedoPath.getString();
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

        auto outputPath(filePath.withoutExtension().withExtension(".gek"));
        LockedWrite{ std::cout } << "Writing: " << outputPath.getString();
        outputPath.getParentPath().createChain();

        FILE* file = nullptr;
        _wfopen_s(&file, outputPath.getWideString().data(), L"wb");
        if (file == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to create output file";
        }

        std::set<std::string> materialList;
        for (auto& mesh : model.meshList)
        {
            materialList.insert(mesh.material);
        }

        Header header;
        header.materialCount = materialList.size();
        header.meshCount = model.meshList.size();
        fwrite(&header, sizeof(Header), 1, file);
        for (auto const& material : materialList)
        {
            Header::Material materialHeader;
            std::strncpy(materialHeader.name, material.data(), 63);
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        for (auto const& mesh : model.meshList)
        {
            LockedWrite{ std::cout } << "-   " << mesh.material;
            LockedWrite{ std::cout } << "    " << mesh.pointList.size() << "  vertices";
            LockedWrite{ std::cout } << "    " << mesh.faceList.size() << " faces";

            auto materialSearch = materialList.find(mesh.material);

            Header::Mesh meshHeader;
            meshHeader.materialIndex = std::distance(std::begin(materialList), materialSearch);
            meshHeader.faceCount = mesh.faceList.size();
            meshHeader.pointCount = mesh.pointList.size();
            fwrite(&meshHeader, sizeof(Header::Mesh), 1, file);
            fwrite(mesh.faceList.data(), sizeof(Mesh::Face), meshHeader.faceCount, file);
            fwrite(mesh.pointList.data(), sizeof(Math::Float3), meshHeader.pointCount, file);
        }

        fclose(file);
    }

    return 0;
}
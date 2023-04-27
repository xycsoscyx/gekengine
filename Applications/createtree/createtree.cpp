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

#ifdef _WIN32
#include <Windows.h>
#endif

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

bool GetModels(Context *context, Parameters const& parameters, aiScene const* inputScene, aiNode const* inputNode, aiMatrix4x4 const& parentTransform, Model& model, std::function<std::string(const std::string&, const std::string&)> findMaterialForMesh)
{
    if (inputNode == nullptr)
    {
        context->log(Context::Error, "Invalid scene node");
        return false;
    }

    aiMatrix4x4 transform(parentTransform * inputNode->mTransformation);
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            context->log(Context::Error, "Invalid mesh list");
            return false;
        }

        std::string name = inputNode->mName.C_Str();
        context->log(Context::Info, "Found Assimp Model: {}", name);
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                context->log(Context::Error, "Invalid mesh index");
                continue;
            }

            const aiMesh* inputMesh = inputScene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh face list");
                    continue;
                }

                if (inputMesh->mVertices == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh vertex list");
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
                        context->log(Context::Error, "Skipping non-triangular face, face: {}, {} indices", faceIndex, face.mNumIndices);
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
            context->log(Context::Error, "Invalid child list");
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            if (!GetModels(context, parameters, inputScene, inputNode->mChildren[childIndex], transform, model, findMaterialForMesh))
            {
                return false;
            }
        }
    }

    return true;
}

int main(int argumentCount, char const * const argumentList[], char const * const environmentVariableList)
{
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
            arguments.push_back(argumentList[argumentIndex]);
        }

        program.parse_args(arguments);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    Parameters parameters;
    parameters.sourceName = program.get<std::string>("--input");
    parameters.targetName = program.get<std::string>("--output");
    parameters.feetPerUnit = (1.0f / program.get<float>("--unitsperfoot"));

    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto rootPath(pluginPath.getParentPath());
    auto cachePath(rootPath / "cache");
    cachePath.setWorkingDirectory();

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    ContextPtr context(Context::Create(nullptr));
    if (context)
    {
        context->log(Context::Info, "GEK Tree Converter");
        context->setCachePath(cachePath);

        auto gekDataPath = std::getenv("gek_data_path");
        if (gekDataPath)
        {
            context->addDataPath(gekDataPath);
        }

        context->addDataPath(rootPath / "data");
        context->addDataPath(rootPath.getString());

        aiLogStream logStream;
        logStream.callback = [](char const* message, char* user) -> void
        {
            Context* context = reinterpret_cast<Context*>(user);
            context->log(Context::Info, message);
        };

        logStream.user = reinterpret_cast<char*>(context.get());
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

        auto filePath = context->findDataPath(FileSystem::CreatePath("physics", parameters.sourceName));
        context->log(Context::Info, "Loading: {}", filePath.getString());
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to load scene with Assimp");
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_Triangulate);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to apply post processing with Assimp");
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_SplitLargeMeshes);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to apply post processing with Assimp");
            return -__LINE__;
        }

        if (!inputScene->HasMeshes())
        {
            context->log(Context::Error, "Scene has no meshes");
            return -__LINE__;
        }

        if (!inputScene->HasMaterials())
        {
            context->log(Context::Error, "Exporting to model requires materials in scene");
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

            JSON::Object materialNode = JSON::Load(filePath);
            auto& shaderNode = materialNode["shader"];
            auto& dataNode = shaderNode["data"];
            auto& albedoNode = dataNode["albedo"];
            auto albedoFile = JSON::Value(albedoNode, "file", String::Empty);
            auto albedoPath = removeRoot("textures", context->findDataPath(albedoFile));

            context->log(Context::Info, "Found material: {}, , with albedo: {}", filePath.getString(), albedoFile);
            albedoToMaterialMap[String::GetLower(albedoFile)] = String::GetLower(removeRoot("materials", filePath).getString());
            return true;
        };

        context->findDataFiles("materials", findMaterials);
        if (albedoToMaterialMap.empty())
        {
            context->log(Context::Error, "Unable to locate any materials");
            return -__LINE__;
        }

        auto findMaterialForMesh = [&](const FileSystem::Path& sourceName, std::string diffuseName) -> std::string
        {
            context->log(Context::Info, "> Searching for : {}, {}", diffuseName, (filePath / diffuseName).getString());

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
                    context->log(Context::Info, "  Found material for albedo: {}, belongs to {}", albedoPath.getString(), albedoSearch->second);
                    return albedoSearch->second;
                }
            }

            context->log(Context::Error, "! Unable to find material for albedo: {}, {}", diffuseName, albedoPath.getString());
            return "";
        };

        Model model;
        aiMatrix4x4 identity;
        if (!GetModels(context.get(), parameters, inputScene, inputScene->mRootNode, identity, model, findMaterialForMesh))
        {
            return -__LINE__;
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(inputScene);

        context->log(Context::Info, "- Num. Meshes: {}", model.meshList.size());
        context->log(Context::Info, "- Size: Minimum[{}, {}, {}]", model.boundingBox.minimum.x, model.boundingBox.minimum.y, model.boundingBox.minimum.z);
        context->log(Context::Info, "- Size: Maximum[{}, {}, {}]", model.boundingBox.maximum.x, model.boundingBox.maximum.y, model.boundingBox.maximum.z);

        auto outputPath(filePath.withoutExtension().withExtension(".gek"));
        context->log(Context::Info, "Writing: {}", outputPath.getString());
        outputPath.getParentPath().createChain();

        std::ofstream file;
        file.open(outputPath.getString().data(), std::ios::out | std::ios::binary);
        if (file.is_open())
        {
            std::set<std::string> materialList;
            for (auto& mesh : model.meshList)
            {
                materialList.insert(mesh.material);
            }

            Header header;
            header.materialCount = materialList.size();
            header.meshCount = model.meshList.size();
            FileSystem::Write(file, &header, 1);
            for (auto const& material : materialList)
            {
                Header::Material materialHeader;
                std::strncpy(materialHeader.name, material.data(), 63);
                FileSystem::Write(file, &materialHeader, 1);
            }

            for (auto const& mesh : model.meshList)
            {
                context->log(Context::Info, "Material: {}", mesh.material);
                context->log(Context::Info, "Num. Points: {}", mesh.pointList.size());
                context->log(Context::Info, "Num. Faces: {}", mesh.faceList.size());

                auto materialSearch = materialList.find(mesh.material);

                Header::Mesh meshHeader;
                meshHeader.materialIndex = std::distance(std::begin(materialList), materialSearch);
                meshHeader.faceCount = mesh.faceList.size();
                meshHeader.pointCount = mesh.pointList.size();
                FileSystem::Write(file, &meshHeader, 1);
                FileSystem::Write(file, mesh.faceList.data(), meshHeader.faceCount);
                FileSystem::Write(file, mesh.pointList.data(), meshHeader.pointCount); 
            }

            file.close();
        }
        else
        {
            context->log(Context::Error, "Unable to create output file");
        }
    }

    return 0;
}
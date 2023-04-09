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
    struct Mesh
    {
        char material[64] = "";
        uint32_t vertexCount = 0;
        uint32_t faceCount = 0;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 0;
    uint16_t version = 8;

    Shapes::AlignedBox boundingBox;

    uint32_t meshCount = 0;
};

struct Mesh
{
    struct Face
    {
        uint16_t data[3];
        uint16_t &operator [] (size_t index)
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
    std::vector<Math::Float2> texCoordList;
    std::vector<Math::Float3> tangentList;
    std::vector<Math::Float3> biTangentList;
    std::vector<Math::Float3> normalList;
    std::vector<Face> faceList;
};

struct Model
{
    std::string name;
    Shapes::AlignedBox boundingBox;
    std::vector<Mesh> meshList;
};

using ModelList = std::vector<Model>;

struct Parameters
{
    std::string sourceName;
    float feetPerUnit = 1.0f;
    bool flipCoords = false;
    bool flipWinding = false;
    float smoothingAngle = 80.0f;
};

bool GetModels(Parameters const &parameters, aiScene const *inputScene, aiNode const *inputNode, aiMatrix4x4 const &parentTransform, ModelList &modelList, std::function<std::string(const std::string &, const std::string &)> findMaterialForMesh)
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

        Model model;
        model.name = inputNode->mName.C_Str();
		if (model.name.empty())
		{
			model.name = std::format("model_{}", modelList.size());
		}

		LockedWrite{ std::cout } << "Found Assimp Model: " << model.name;
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                LockedWrite{ std::cerr } << "Invalid mesh index";
                continue;
            }

            const aiMesh *inputMesh = inputScene->mMeshes[nodeMeshIndex];
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

                if (inputMesh->mTextureCoords[0] == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh texture coordinate list";
                    continue;
                }

                if (inputMesh->mTangents == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh tangent list";
                    continue;
                }

                if (inputMesh->mBitangents == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh bitangent list";
                    continue;
                }

                if (inputMesh->mNormals == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh normal list";
                    continue;
                }

                Mesh mesh;

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string diffuseName = sceneDiffuseMaterial.C_Str();
                mesh.material = findMaterialForMesh(parameters.sourceName, diffuseName);
                if (mesh.material.empty())
                {
                    LockedWrite{ std::cerr } << "Unable to find material for mesh " << inputMesh->mName.C_Str();
                    continue;
                }

                mesh.pointList.resize(inputMesh->mNumVertices);
                mesh.texCoordList.resize(inputMesh->mNumVertices);
                mesh.tangentList.resize(inputMesh->mNumVertices);
                mesh.biTangentList.resize(inputMesh->mNumVertices);
                mesh.normalList.resize(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertex = inputMesh->mVertices[vertexIndex];
                    aiTransformVecByMatrix4(&vertex, &transform);
                    mesh.pointList[vertexIndex].set(vertex.x, vertex.y, vertex.z);
                    mesh.pointList[vertexIndex] *= parameters.feetPerUnit;
                    model.boundingBox.extend(mesh.pointList[vertexIndex]);

                    mesh.texCoordList[vertexIndex].set(
                        inputMesh->mTextureCoords[0][vertexIndex].x,
                        inputMesh->mTextureCoords[0][vertexIndex].y);

                    mesh.tangentList[vertexIndex].set(
                        inputMesh->mTangents[vertexIndex].x,
                        inputMesh->mTangents[vertexIndex].y,
                        inputMesh->mTangents[vertexIndex].z);

                    mesh.biTangentList[vertexIndex].set(
                        inputMesh->mBitangents[vertexIndex].x,
                        inputMesh->mBitangents[vertexIndex].y,
                        inputMesh->mBitangents[vertexIndex].z);

                    mesh.normalList[vertexIndex].set(
                        inputMesh->mNormals[vertexIndex].x,
                        inputMesh->mNormals[vertexIndex].y,
                        inputMesh->mNormals[vertexIndex].z);
                }

                mesh.faceList.reserve(inputMesh->mNumFaces);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = inputMesh->mFaces[faceIndex];
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

            modelList.push_back(model);
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
            if (!GetModels(parameters, inputScene, inputNode->mChildren[childIndex], transform, modelList, findMaterialForMesh))
            {
                return false;
            }
        }
    }

    return true;
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{ std::cout } << "GEK Model Converter";

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
        else if (arguments[0] == "-flipcoords")
        {
            parameters.flipCoords = true;
        }
        else if (arguments[0] == "-flipwinding")
        {
            parameters.flipWinding = true;
        }
        else if (arguments[0] == "-smoothangle")
        {
            if (arguments.size() != 2)
            {
                LockedWrite{ std::cerr } << "Missing parameters for smoothAngle";
                return -__LINE__;
            }

            parameters.smoothingAngle = String::Convert(arguments[1], 80.0f);
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
    logStream.callback = [](char const *message, char *user) -> void
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
        (parameters.flipCoords ? aiProcess_FlipUVs : 0) |
        aiProcess_RemoveComponent |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        aiProcess_ValidateDataStructure;

    aiPropertyStore *propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, parameters.smoothingAngle);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);

    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto rootPath(pluginPath.getParentPath());
    auto cachePath(rootPath / "cache"sv);
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

        context->addDataPath(rootPath / "data"sv);
        context->addDataPath(rootPath.getString());

        auto filePath = context->findDataPath(FileSystem::CreatePath("models", parameters.sourceName));

        LockedWrite{ std::cout } << "Loading: " << filePath.getString();
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
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

            aiProcess_GenUVCoords |
            aiProcess_TransformUVCoords |
            0,

            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            0,

            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            0,

            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            //aiProcess_OptimizeGraph,
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

        auto texturesPath(context->findDataPath("textures").getString());
        auto engineIndex = texturesPath.find("gek engine");
        if (engineIndex != std::string::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.substr(engineIndex);
        }

        std::function<FileSystem::Path (const char*, FileSystem::Path const &)> removeRoot = [](const char *location, FileSystem::Path const & path) -> FileSystem::Path
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

        ModelList modelList;
        aiMatrix4x4 identity;
        if (!GetModels(parameters, inputScene, inputScene->mRootNode, identity, modelList, findMaterialForMesh))
        {
            return -__LINE__;
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(inputScene);

        LockedWrite{ std::cout } << "> Num. Models: " << modelList.size();
        for (auto& model : modelList)
        {
            auto modelName(model.name);
            for (auto replacement : { "$", "<", ">" })
            {
                String::Replace(modelName, replacement, "");
            }

            auto outputPath((filePath.withoutExtension() / modelName).withExtension(".gek"));
            LockedWrite{ std::cout } << "> Model: " << model.name << ": " << outputPath.getString();
            LockedWrite{ std::cout } << "   Num. Meshes: " << model.meshList.size();
            LockedWrite{ std::cout } << "   Size: Minimum[" << model.boundingBox.minimum.x << ", " << model.boundingBox.minimum.y << ", " << model.boundingBox.minimum.z << "]";
            LockedWrite{ std::cout } << "   Size: Maximum[" << model.boundingBox.maximum.x << ", " << model.boundingBox.maximum.y << ", " << model.boundingBox.maximum.z << "]";

            filePath.withoutExtension().createChain();
            auto file = fopen(outputPath.getString().data(), "wb");
            if (file == nullptr)
            {
                LockedWrite{ std::cerr } << "Unable to create output file";
                return -__LINE__;
            }

            Header header;
            header.meshCount = model.meshList.size();
            header.boundingBox = model.boundingBox;
            fwrite(&header, sizeof(Header), 1, file);

            for (auto& mesh : model.meshList)
            {
                LockedWrite{ std::cout } << "   > Mesh: " << mesh.material;
                LockedWrite{ std::cout } << "      Num. Vertices: " << mesh.pointList.size();
                LockedWrite{ std::cout } << "      Num. Faces: " << mesh.faceList.size();

                Header::Mesh meshHeader;
                std::strncpy(meshHeader.material, mesh.material.data(), 63);
                meshHeader.vertexCount = mesh.pointList.size();
                meshHeader.faceCount = mesh.faceList.size();
                fwrite(&meshHeader, sizeof(Header::Mesh), 1, file);
            }

            for (auto& mesh : model.meshList)
            {
                fwrite(mesh.faceList.data(), sizeof(Mesh::Face), mesh.faceList.size(), file);
                fwrite(mesh.pointList.data(), sizeof(Math::Float3), mesh.pointList.size(), file);
                fwrite(mesh.texCoordList.data(), sizeof(Math::Float2), mesh.texCoordList.size(), file);
                fwrite(mesh.tangentList.data(), sizeof(Math::Float3), mesh.tangentList.size(), file);
                fwrite(mesh.biTangentList.data(), sizeof(Math::Float3), mesh.biTangentList.size(), file);
                fwrite(mesh.normalList.data(), sizeof(Math::Float3), mesh.normalList.size(), file);
            }

            fclose(file);
        }
    }

    return 0;
}
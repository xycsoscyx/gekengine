#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include <argparse/argparse.hpp>
#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <mikktspace.h>
#include <fmt/format.h>
#include <unordered_map>
#include <algorithm>
#include <string.h>
#include <vector>
#include <map>

#ifdef _WIN32
#include <Windows.h>
#endif

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
    std::string material;
    std::vector<Math::Float3> pointList;
    std::vector<Math::Float2> texCoordList;
    std::vector<Math::Float4> tangentList;
    std::vector<Math::Float3> normalList;
};

struct IndexedMesh
    : public Mesh
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

    std::vector<Face> faceList;
};

struct Model
{
    std::string name;
    Shapes::AlignedBox boundingBox;
    std::vector<Mesh> meshList;
    std::vector<IndexedMesh> indexedMeshList;
};

using ModelList = std::vector<Model>;

struct Parameters
{
    std::string sourceName;
    float feetPerUnit;
    bool generateSmoothNormals;
    float smoothingAngle;
    bool saveAsCode;
};

bool GetModels(Context *context, Parameters const &parameters, aiScene const *inputScene, aiNode const *inputNode, aiMatrix4x4 const &accumulatedTransform, ModelList &modelList, std::function<std::string(const std::string &, const std::string &)> findMaterialForMesh)
{
    if (inputNode == nullptr)
    {
        context->log(Context::Error, "Invalid scene node");
        return false;
    }

    auto transform = accumulatedTransform * inputNode->mTransformation;
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            context->log(Context::Error, "Invalid mesh list");
            return false;
        }

        Math::Float4x4 localTransform(&transform.a1);
        localTransform.transpose();

        Model model;
        model.name = inputNode->mName.C_Str();
		if (model.name.empty())
		{
			model.name = fmt::format("model_{}", modelList.size());
		}

		context->log(Context::Info, "Found Assimp Model: {}", model.name);
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                context->log(Context::Error, "Invalid mesh index");
                continue;
            }

            const aiMesh *inputMesh = inputScene->mMeshes[nodeMeshIndex];
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

                if (inputMesh->mTextureCoords[0] == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh texture coordinate list");
                    continue;
                }

                if (inputMesh->mNormals == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh normal list");
                    continue;
                }

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string diffuseName = sceneDiffuseMaterial.C_Str();
                auto material = findMaterialForMesh(parameters.sourceName, diffuseName);
                if (material.empty())
                {
                    context->log(Context::Error, "Unable to find material for mesh {}", inputMesh->mName.C_Str());
                    continue;
                }

                context->log(Context::Info, "- Material: {}", material);
                context->log(Context::Info, "- Num. Faces: {}", inputMesh->mNumFaces);
                context->log(Context::Info, "- Num. Vertex: {}", inputMesh->mNumVertices);

                IndexedMesh indexedMesh;
                indexedMesh.material = material;
                indexedMesh.faceList.reserve(inputMesh->mNumFaces);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace& face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        context->log(Context::Error, "Skipping non-triangular face, face: {}, {} indices", faceIndex, face.mNumIndices);
                        continue;
                    }

                    IndexedMesh::Face meshFace;
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        meshFace[edgeIndex] = face.mIndices[edgeIndex];
                    }

                    indexedMesh.faceList.push_back(meshFace);
                }

                indexedMesh.pointList.resize(inputMesh->mNumVertices);
                indexedMesh.texCoordList.resize(inputMesh->mNumVertices);
                indexedMesh.tangentList.resize(inputMesh->mNumVertices);
                indexedMesh.normalList.resize(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    Math::Float3 position(inputMesh->mVertices[vertexIndex].x, inputMesh->mVertices[vertexIndex].y, inputMesh->mVertices[vertexIndex].z);
                    Math::Float2 texCoord(inputMesh->mTextureCoords[0][vertexIndex].x, inputMesh->mTextureCoords[0][vertexIndex].y);
                    Math::Float3 tangent(inputMesh->mTangents[vertexIndex].x, inputMesh->mTangents[vertexIndex].y, inputMesh->mTangents[vertexIndex].z);
                    Math::Float3 normal(inputMesh->mNormals[vertexIndex].x, inputMesh->mNormals[vertexIndex].y, inputMesh->mNormals[vertexIndex].z);

                    indexedMesh.pointList[vertexIndex] = localTransform.transform(position) * parameters.feetPerUnit;
                    indexedMesh.texCoordList[vertexIndex] = texCoord;
                    indexedMesh.tangentList[vertexIndex].set(localTransform.rotate(normal), 1.0f);
                    indexedMesh.normalList[vertexIndex] = localTransform.rotate(normal);
                }

                for (const auto& point : indexedMesh.pointList)
                {
                    model.boundingBox.extend(point);
                }

                Mesh mesh;
                mesh.material = material;
                mesh.pointList.reserve(inputMesh->mNumFaces * 3);
                mesh.texCoordList.reserve(inputMesh->mNumFaces * 3);
                mesh.tangentList.reserve(inputMesh->mNumFaces * 3);
                mesh.normalList.reserve(inputMesh->mNumFaces * 3);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace& face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        context->log(Context::Error, "Skipping non-triangular face, face: {}, {} indices", faceIndex, face.mNumIndices);
                        continue;
                    }

                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        auto vertexIndex = face.mIndices[edgeIndex];
                        Math::Float3 position(inputMesh->mVertices[vertexIndex].x, inputMesh->mVertices[vertexIndex].y, inputMesh->mVertices[vertexIndex].z);
                        Math::Float2 texCoord(inputMesh->mTextureCoords[0][vertexIndex].x, inputMesh->mTextureCoords[0][vertexIndex].y);
                        Math::Float3 tangent(inputMesh->mTangents[vertexIndex].x, inputMesh->mTangents[vertexIndex].y, inputMesh->mTangents[vertexIndex].z);
                        Math::Float3 normal(inputMesh->mNormals[vertexIndex].x, inputMesh->mNormals[vertexIndex].y, inputMesh->mNormals[vertexIndex].z);

                        mesh.pointList.push_back(localTransform.transform(position) * parameters.feetPerUnit);
                        mesh.texCoordList.push_back(texCoord);
                        mesh.tangentList.push_back(Math::Float4(localTransform.rotate(normal), 1.0f));
                        mesh.normalList.push_back(localTransform.rotate(normal));
                    }
                }

                for (const auto& point : mesh.pointList)
                {
                    model.boundingBox.extend(point);
                }

                SMikkTSpaceInterface mikkInterface;
                mikkInterface.m_getNumFaces = [](const SMikkTSpaceContext* mikkContext) -> int
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    return mesh.pointList.size() / 3;
                };

                mikkInterface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* mikkContext, const int faceIndex) -> int
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    return 3;
                };

                mikkInterface.m_getPosition = [](const SMikkTSpaceContext* mikkContext, float position[], const int faceIndex, const int vertexIndex) -> void
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    position[0] = mesh.pointList[(faceIndex * 3) + vertexIndex].x;
                    position[1] = mesh.pointList[(faceIndex * 3) + vertexIndex].y;
                    position[2] = mesh.pointList[(faceIndex * 3) + vertexIndex].z;
                };

                mikkInterface.m_getNormal = [](const SMikkTSpaceContext* mikkContext, float normal[], const int faceIndex, const int vertexIndex) -> void
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    normal[0] = mesh.normalList[(faceIndex * 3) + vertexIndex].x;
                    normal[1] = mesh.normalList[(faceIndex * 3) + vertexIndex].y;
                    normal[2] = mesh.normalList[(faceIndex * 3) + vertexIndex].z;
                };

                mikkInterface.m_getTexCoord = [](const SMikkTSpaceContext* mikkContext, float texCoord[], const int faceIndex, const int vertexIndex) -> void
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    texCoord[0] = mesh.texCoordList[(faceIndex * 3) + vertexIndex].x;
                    texCoord[1] = mesh.texCoordList[(faceIndex * 3) + vertexIndex].y;
                };

                mikkInterface.m_setTSpaceBasic = [](const SMikkTSpaceContext* mikkContext, const float tangent[], const float sign, const int faceIndex, const int vertexIndex) -> void
                {
                    auto& mesh = *reinterpret_cast<Mesh*>(mikkContext->m_pUserData);
                    mesh.tangentList[(faceIndex * 3) + vertexIndex].x = tangent[0];
                    mesh.tangentList[(faceIndex * 3) + vertexIndex].y = tangent[1];
                    mesh.tangentList[(faceIndex * 3) + vertexIndex].z = tangent[2];
                    mesh.tangentList[(faceIndex * 3) + vertexIndex].w = sign;
                };

                mikkInterface.m_setTSpace = nullptr;

                SMikkTSpaceContext mikkContext;
                mikkContext.m_pInterface = &mikkInterface;
                mikkContext.m_pUserData = &mesh;

                genTangSpaceDefault(&mikkContext);

                model.meshList.push_back(mesh);
                model.indexedMeshList.push_back(indexedMesh);
            }

            modelList.push_back(model);
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
            if (!GetModels(context, parameters, inputScene, inputNode->mChildren[childIndex], transform, modelList, findMaterialForMesh))
            {
                return false;
            }
        }
    }

    return true;
}

int main(int argumentCount, char const * const argumentList[])
{
    argparse::ArgumentParser program("GEK Model Converter", "1.0");

    program.add_argument("-i", "--input")
        .required()
        .help("input model");

    program.add_argument("-c", "--codify")
        .help("save output as code")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-s", "--smoothangle")
        .scan<'g', float>()
        .help("smoothing angle")
        .default_value(90.0f);

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
    parameters.feetPerUnit = (1.0f / program.get<float>("--unitsperfoot"));
    parameters.smoothingAngle = program.get<float>("--smoothangle");
    parameters.generateSmoothNormals = program.is_used("--smoothangle");
    parameters.saveAsCode = program.get<bool>("codify");

    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto cachePath(FileSystem::GetCacheFromModule());
    auto rootPath(cachePath.getParentPath());
    cachePath.setWorkingDirectory();

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    ContextPtr context(Context::Create(nullptr));
    if (context)
    {
        context->log(Context::Info, "GEK Model Converter");
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
            //aiComponent_NORMALS |
            //aiComponent_TANGENTS_AND_BITANGENTS |
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
            aiProcess_FlipUVs |
            aiProcess_GenUVCoords |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            //aiProcess_OptimizeGraph,
            0;

        aiPropertyStore* propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, parameters.smoothingAngle);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        //aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        auto filePath = context->findDataPath(FileSystem::CreatePath("models", parameters.sourceName));
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

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_CalcTangentSpace);
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

            JSON::Object materialNode = JSON::Load(filePath);
            auto& shaderNode = materialNode["shader"];
            auto& dataNode = shaderNode["data"];
            auto& albedoNode = dataNode["albedo"];
            FileSystem::Path albedoPath = JSON::Value(albedoNode, "file", String::Empty);
            auto albedoFile = albedoPath.withoutExtension().getString();

            context->log(Context::Info, "Found material: {}, with albedo: {}", filePath.getString(), albedoFile);
            albedoToMaterialMap[String::GetLower(albedoFile)] = String::GetLower(removeRoot("materials", filePath).getString());
            return true;
        };

        context->findDataFiles("materials", findMaterials);
        if (albedoToMaterialMap.empty())
        {
            context->log(Context::Error, "Unable to locate any materials");
            return -__LINE__;
        }

        auto findMaterial = [&](FileSystem::Path albedoPath) -> std::string
        {
            albedoPath = removeRoot("textures", albedoPath);
            context->log(Context::Info, "Searching for albedo: {}", albedoPath.getString());

            auto albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.withoutExtension().getString()));
            if (albedoSearch == std::end(albedoToMaterialMap))
            {
                albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.getString()));
            }

            if (albedoSearch != std::end(albedoToMaterialMap))
            {
                context->log(Context::Info, "Found material for albedo: {} belongs to {}", albedoPath.getString(), albedoSearch->second);
                return albedoSearch->second;
            }

            return String::Empty;
        };

        auto findMaterialForMesh = [&](const FileSystem::Path& sourceName, std::string diffuseName) -> std::string
        {
            context->log(Context::Info, "Searching for: {}, {}", diffuseName, (filePath / diffuseName).getString());

            FileSystem::Path albedoPath = FileSystem::Path(diffuseName).lexicallyRelative("textures");
            albedoPath = filePath.withoutExtension().getFileName() / albedoPath.withoutExtension();
            auto materialName = findMaterial(albedoPath);
            if (!materialName.empty())
            {
                return materialName;
            }

            albedoPath = FileSystem::GetCanonicalPath(filePath / diffuseName);
            materialName = findMaterial(albedoPath);
            if (!materialName.empty())
            {
                return materialName;
            }

            return String::Empty;
        };

        ModelList modelList;
        aiMatrix4x4 rootTransform;
        if (!GetModels(context.get(), parameters, inputScene, inputScene->mRootNode, rootTransform, modelList, findMaterialForMesh))
        {
            return -__LINE__;
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(inputScene);

        context->log(Context::Info, "Num. Models: {}", modelList.size());
        if (parameters.saveAsCode)
        {
            static constexpr std::string_view modelDefinition =
                R"(#ifndef GEK_STATIC_MODEL
#define GEK_STATIC_MODEL
struct StaticModel
{
    std::string material;
    std::vector<uint16_t> indices;
    std::vector<Math::Float3> positions;
    std::vector<Math::Float2> texCoords;
    std::vector<Math::Float4> tangents;
    std::vector<Math::Float3> normals;

    StaticModel(std::string &&material, 
        std::vector<Math::Float3> &&positions,
        std::vector<Math::Float2> &&texCoords,
        std::vector<Math::Float4> &&tangents,
        std::vector<Math::Float3> &&normals)
        : material(std::move(material))
        , indices(std::move(indices))
        , positions(std::move(positions))
        , texCoords(std::move(texCoords))
        , tangents(std::move(tangents))
        , normals(std::move(normals))
    {
    }
};
#endif)";

            auto outputPath(filePath.withoutExtension().withExtension(".h"));
            auto file = fopen(outputPath.getString().data(), "w");
            if (file == nullptr)
            {
                context->log(Context::Error, "Unable to create output file");
                return -__LINE__;
            }

            static constexpr std::string_view modelTemplate =
                R"(static const StaticModel {0}_models[] = {{)";
            std::string code(modelDefinition);
            code += "\n";
            code += fmt::vformat(modelTemplate, fmt::make_format_args(filePath.withoutExtension().getFileName()));
            code += "\n";

            for (auto& model : modelList)
            {
                context->log(Context::Info, "- Model: {}", model.name);
                context->log(Context::Info, "- Num. Meshes: {}", model.meshList.size());
                context->log(Context::Info, "- Size: Minimum[{}, {}, {}]", model.boundingBox.minimum.x, model.boundingBox.minimum.y, model.boundingBox.minimum.z);
                context->log(Context::Info, "- Size: Maximum[{}, {}, {}]", model.boundingBox.maximum.x, model.boundingBox.maximum.y, model.boundingBox.maximum.z);
                for (auto& mesh : model.meshList)
                {
                    String::Replace(mesh.material, "\\", "\\\\");
                    code += fmt::format("   StaticModel(\"{}\",\n       std::vector<Math::Float3>({{ ", mesh.material);
                    for (auto& point : mesh.pointList)
                    {
                        code += fmt::format("Math::Float3({:.5F}f, {:.5F}f, {:.5F}f), ", point.x, point.y, point.z);
                    }

                    code += fmt::format("}}),\n       std::vector<Math::Float2>({{ ");
                    for (auto& texCoord : mesh.texCoordList)
                    {
                        code += fmt::format("Math::Float2({:.5F}f, {:.5F}f), ", texCoord.x, texCoord.y);
                    }

                    code += fmt::format("}}),\n       std::vector<Math::Float4>({{ ");
                    for (auto& tangent : mesh.tangentList)
                    {
                        code += fmt::format("Math::Float4({:.5F}f, {:.5F}f, {:.5F}f, {:.5F}f), ", tangent.x, tangent.y, tangent.z, tangent.w);
                    }

                    code += fmt::format("}}),\n       std::vector<Math::Float3>({{ ");
                    for (auto& normal : mesh.normalList)
                    {
                        code += fmt::format("Math::Float3({:.5F}f, {:.5F}f, {:.5F}f), ", normal.x, normal.y, normal.z);
                    }

                    code += fmt::format("}})),\n");
                }
            }

            code += fmt::format("\n}};");
            fwrite(code.data(), 1, code.size(), file);
            fclose(file);
        }
        else
        {
            for (auto& model : modelList)
            {
                auto modelName(model.name);
                for (auto replacement : { "$", "<", ">" })
                {
                    String::Replace(modelName, replacement, "");
                }

                auto outputPath((filePath.withoutExtension() / modelName).withExtension(".gek"));
                context->log(Context::Info, "- Model: {}, {}", model.name, outputPath.getString());
                context->log(Context::Info, "- Num. Meshes: {}", model.meshList.size());
                context->log(Context::Info, "- Size: Minimum[{}, {}, {}]", model.boundingBox.minimum.x, model.boundingBox.minimum.y, model.boundingBox.minimum.z);
                context->log(Context::Info, "- Size: Maximum[{}, {}, {}]", model.boundingBox.maximum.x, model.boundingBox.maximum.y, model.boundingBox.maximum.z);
                filePath.withoutExtension().createChain();
                auto file = fopen(outputPath.getString().data(), "wb");
                if (file == nullptr)
                {
                    context->log(Context::Info, "Unable to create output file");
                    return -__LINE__;
                }

                Header header;
                header.meshCount = model.meshList.size();
                header.boundingBox = model.boundingBox;
                fwrite(&header, sizeof(Header), 1, file);

                for (auto& mesh : model.meshList)
                {
                    context->log(Context::Info, "- Mesh: {}", mesh.material);
                    context->log(Context::Info, "- Num. Vertices: {}", mesh.pointList.size());
                    //context->log(Context::Info, "- Num. Faces: {}", mesh.faceList.size());

                    Header::Mesh meshHeader;
                    std::strncpy(meshHeader.material, mesh.material.data(), 63);
                    meshHeader.vertexCount = mesh.pointList.size();
                    //meshHeader.faceCount = mesh.faceList.size();
                    fwrite(&meshHeader, sizeof(Header::Mesh), 1, file);
                }

                for (auto& mesh : model.meshList)
                {
                    //fwrite(mesh.faceList.data(), sizeof(Mesh::Face), mesh.faceList.size(), file);
                    fwrite(mesh.pointList.data(), sizeof(Math::Float3), mesh.pointList.size(), file);
                    fwrite(mesh.texCoordList.data(), sizeof(Math::Float2), mesh.texCoordList.size(), file);
                    fwrite(mesh.tangentList.data(), sizeof(Math::Float4), mesh.tangentList.size(), file);
                    fwrite(mesh.normalList.data(), sizeof(Math::Float3), mesh.normalList.size(), file);
                }

                fclose(file);
            }
        }
    }

    return 0;
}
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

#include <mikktspace.h>

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
    float feetPerUnit = 1.0f;
    bool generateSmoothNormals = false;
    float smoothingAngle = 90.0f;
    bool saveAsCode = false;
};

bool GetModels(Parameters const &parameters, aiScene const *inputScene, aiNode const *inputNode, aiMatrix4x4 const &accumulatedTransform, ModelList &modelList, std::function<std::string(const std::string &, const std::string &)> findMaterialForMesh)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << "Invalid scene node";
        return false;
    }

    auto transform = accumulatedTransform * inputNode->mTransformation;
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid mesh list";
            return false;
        }

        Math::Float4x4 localTransform(&transform.a1);
        localTransform.transpose();

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

                if (inputMesh->mNormals == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh normal list";
                    continue;
                }

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string diffuseName = sceneDiffuseMaterial.C_Str();
                auto material = findMaterialForMesh(parameters.sourceName, diffuseName);
                if (material.empty())
                {
                    LockedWrite{ std::cerr } << "Unable to find material for mesh " << inputMesh->mName.C_Str();
                    continue;
                }

                LockedWrite{ std::cout } << " -> Material: " << material;
                LockedWrite{ std::cerr } << " -> Num. Faces: " << inputMesh->mNumFaces;
                LockedWrite{ std::cerr } << " -> Num. Vertex: " << inputMesh->mNumVertices;

                IndexedMesh indexedMesh;
                indexedMesh.material = material;
                indexedMesh.faceList.reserve(inputMesh->mNumFaces);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace& face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        LockedWrite{ std::cerr } << "Skipping non-triangular face, face: " << faceIndex << ": " << face.mNumIndices << " indices";
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
                    indexedMesh.tangentList[vertexIndex].xyz = localTransform.rotate(normal);
                    indexedMesh.tangentList[vertexIndex].w = 1.0f;
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
                        LockedWrite{ std::cerr } << "Skipping non-triangular face, face: " << faceIndex << ": " << face.mNumIndices << " indices";
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
        else if (arguments[0] == "-smoothangle")
        {
            if (arguments.size() != 2)
            {
                LockedWrite{ std::cerr } << "Missing parameters for smoothAngle";
                return -__LINE__;
            }

            parameters.smoothingAngle = String::Convert(arguments[1], parameters.smoothingAngle);
        }
        else if (arguments[0] == "-unitsinfoot")
        {
            if (arguments.size() != 2)
            {
                LockedWrite{ std::cerr } << "Missing parameters for unitsInFoot";
                return -__LINE__;
            }

            parameters.generateSmoothNormals = true;
			parameters.feetPerUnit = (1.0f / String::Convert(arguments[1], 1.0f));
        }
        else if (arguments[0] == "-code")
        {
            parameters.saveAsCode = true;
        }
    }

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

        auto filePath = context->findDataPath(FileSystem::CreatePath("models", parameters.sourceName));

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

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_CalcTangentSpace);
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
            FileSystem::Path albedoPath = albedoNode.getMember("file").convert(String::Empty);
            auto albedoFile = albedoPath.withoutExtension().getString();

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

        auto findMaterial = [&](FileSystem::Path albedoPath) -> std::string
        {
            albedoPath = removeRoot("textures", albedoPath);
            LockedWrite{ std::cout } << "Searching for albedo: " << albedoPath.getString();

            auto albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.withoutExtension().getString()));
            if (albedoSearch == std::end(albedoToMaterialMap))
            {
                albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.getString()));
            }

            if (albedoSearch != std::end(albedoToMaterialMap))
            {
                LockedWrite{ std::cout } << "Found material for albedo: " << albedoPath.getString() << " belongs to " << albedoSearch->second;
                return albedoSearch->second;
            }

            return String::Empty;
        };

        auto findMaterialForMesh = [&](const FileSystem::Path& sourceName, std::string diffuseName) -> std::string
        {
            LockedWrite{ std::cout } << "Searching for: " << diffuseName << ", " << (filePath / diffuseName).getString();

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
        if (!GetModels(parameters, inputScene, inputScene->mRootNode, rootTransform, modelList, findMaterialForMesh))
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

            auto outputPath((filePath.withoutExtension() / modelName).withExtension(parameters.saveAsCode ? ".h" : ".gek"));
            LockedWrite{ std::cout } << "> Model: " << model.name << ": " << outputPath.getString();
            LockedWrite{ std::cout } << "   Num. Meshes: " << model.meshList.size();
            LockedWrite{ std::cout } << "   Size: Minimum[" << model.boundingBox.minimum.x << ", " << model.boundingBox.minimum.y << ", " << model.boundingBox.minimum.z << "]";
            LockedWrite{ std::cout } << "   Size: Maximum[" << model.boundingBox.maximum.x << ", " << model.boundingBox.maximum.y << ", " << model.boundingBox.maximum.z << "]";

            filePath.withoutExtension().createChain();
            if (parameters.saveAsCode)
            {
                auto modelName = filePath.withoutExtension().getFileName();
                auto file = fopen(outputPath.getString().data(), "w");
                if (file == nullptr)
                {
                    LockedWrite{ std::cerr } << "Unable to create output file";
                    return -__LINE__;
                }

                static constexpr std::string_view codeTemplate =
R"(struct StaticModel_{0}
{{
    std::string material;
    std::vector<uint16_t> indices;
    std::vector<Math::Float3> positions;
    std::vector<Math::Float2> texCoords;
    std::vector<Math::Float4> tangents;
    std::vector<Math::Float3> normals;

    StaticModel_{0}(std::string &&material, 
        std::vector<uint16_t> &&indices,
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
    {{
    }}
}} {0}_models[] =
{{
)";

                std::string code = std::vformat(codeTemplate, std::make_format_args(modelName));
                for (auto& mesh : model.indexedMeshList)
                {
                    String::Replace(mesh.material, "\\", "\\\\");
                    code += std::format("   StaticModel_{}(\"{}\",\n       std::vector<uint16_t>({{ ", modelName, mesh.material);
                    for (auto &face : mesh.faceList)
                    {
                        code += std::format("{}, {}, {}, ", face[0], face[1], face[2]);
                    }

                    code += std::format("}}),\n       std::vector<Math::Float3>({{ ");
                    for (auto& point : mesh.pointList)
                    {
                        code += std::format("Math::Float3({}, {}, {}), ", point.x, point.y, point.z);
                    }

                    code += std::format("}}),\n       std::vector<Math::Float2>({{ ");
                    for (auto& texCoord : mesh.texCoordList)
                    {
                        code += std::format("Math::Float2({}, {}), ", texCoord.x, texCoord.y);
                    }

                    code += std::format("}}),\n       std::vector<Math::Float4>({{ ");
                    for (auto& tangent : mesh.tangentList)
                    {
                        code += std::format("Math::Float4({}, {}, {}, {}), ", tangent.x, tangent.y, tangent.z, tangent.w);
                    }

                    code += std::format("}}),\n       std::vector<Math::Float3>({{ ");
                    for (auto& normal : mesh.pointList)
                    {
                        code += std::format("Math::Float3({}, {}, {}), ", normal.x, normal.y, normal.z);
                    }

                    code += std::format("}})),\n");
                }

                code += std::format("}};");
                fwrite(code.data(), 1, code.size(), file);
                fclose(file);
            }
            else
            {
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
                    //LockedWrite{ std::cout } << "      Num. Faces: " << mesh.faceList.size();

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
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

    uint32_t meshCount;
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
    };

    std::string diffuse;
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
    float feetPerUnit = 1.0f;
};

bool GetModels(Parameters const &parameters, aiScene const *inputScene, aiNode const *inputNode, ModelList &modelList)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << "Invalid scene node";
        return false;
    }

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
			model.name = String::Format("model_{}", modelList.size());
		}

		LockedWrite{ std::cout } << "Found Assimp Model: " << inputNode->mName.C_Str();
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                LockedWrite{ std::cerr } << "Invalid mesh index";
                return false;
            }

            const aiMesh *inputMesh = inputScene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh face list";
                    return false;
                }

                if (inputMesh->mVertices == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh vertex list";
                    return false;
                }

                if (inputMesh->mTextureCoords[0] == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh texture coordinate list";
                    return false;
                }

                if (inputMesh->mTangents == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh tangent list";
                    return false;
                }

                if (inputMesh->mBitangents == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh bitangent list";
                    return false;
                }

                if (inputMesh->mNormals == nullptr)
                {
                    LockedWrite{ std::cerr } << "Invalid inputMesh normal list";
                    return false;
                }

                Mesh mesh;

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                mesh.diffuse = sceneDiffuseMaterial.C_Str();

                mesh.pointList.resize(inputMesh->mNumVertices);
                mesh.texCoordList.resize(inputMesh->mNumVertices);
                mesh.tangentList.resize(inputMesh->mNumVertices);
                mesh.biTangentList.resize(inputMesh->mNumVertices);
                mesh.normalList.resize(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    mesh.pointList[vertexIndex].set(
                        (inputMesh->mVertices[vertexIndex].x * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].y * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].z * parameters.feetPerUnit));
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
            if (!GetModels(parameters, inputScene, inputNode->mChildren[childIndex], modelList))
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

    FileSystem::Path sourceName;
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
            LockedWrite{ std::cerr } << "No arguments specified for command line parameter";
            return -__LINE__;
        }

        if (arguments[0] == "-source" && ++argumentIndex < argumentCount)
        {
            sourceName = String::Narrow(argumentList[argumentIndex]);
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
                LockedWrite{ std::cerr } << "Missing parameters for smoothAngle";
                return -__LINE__;
            }

			smoothingAngle = String::Convert(arguments[1], 80.0f);
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
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        0;

    unsigned int textureProcessFlags =
        aiProcess_GenUVCoords |
        aiProcess_TransformUVCoords |
        0;

    unsigned int tangentProcessFlags =
        aiProcess_JoinIdenticalVertices |
        aiProcess_FindInvalidData |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        //aiProcess_OptimizeGraph |
        0;

    aiPropertyStore *propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
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
		dataPath = FileSystem::CombinePaths(rootPath, "Data");
	}

    auto sourcePath(FileSystem::CombinePaths(dataPath, "models", sourceName.getString()));
    auto inputScene = aiImportFileExWithProperties(sourcePath.getString().data(), importFlags, nullptr, propertyStore);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
        return -__LINE__;
    }

    inputScene = aiApplyPostProcessing(inputScene, textureProcessFlags);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to apply texture post processing with Assimp";
        return -__LINE__;
    }

    inputScene = aiApplyPostProcessing(inputScene, tangentProcessFlags);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to apply tangent post processing with Assimp";
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

    ModelList modelList;
    if (!GetModels(parameters, inputScene, inputScene->mRootNode, modelList))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(inputScene);

	std::string texturesPath(String::GetLower(FileSystem::CombinePaths(dataPath, "Textures").getString()));
    auto materialsPath(FileSystem::CombinePaths(dataPath, "Materials").getString());

	std::map<std::string, std::string> diffuseToMaterialMap;
    std::function<bool(FileSystem::Path const &)> findMaterials;
    findMaterials = [&](FileSystem::Path const &filePath) -> bool
    {
        if (filePath.isDirectory())
        {
            filePath.findFiles(findMaterials);
        }
        else if (filePath.isFile())
        {
            JSON materialNode;
            materialNode.load(filePath);
            auto shaderNode = materialNode.get("shader");
            auto dataNode = shaderNode.get("data");
            auto albedoNode = dataNode.get("albedo");
            std::string albedoPath(albedoNode.get("file").as(String::Empty));
            std::string materialName(String::GetLower(filePath.withoutExtension().getString().substr(materialsPath.size() + 1)));
            diffuseToMaterialMap[albedoPath] = materialName;
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

	LockedWrite{ std::cout } << "> Num. Models: " << modelList.size();

    auto findMaterialForDiffuse = [&](std::string const &diffuse) -> std::string
    {
        FileSystem::Path diffusePath(diffuse);
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

        auto materialAlebedoSearch = diffuseToMaterialMap.find(albedoName);
        if (materialAlebedoSearch == std::end(diffuseToMaterialMap))
        {
            LockedWrite{ std::cerr } << "! Unable to find material for albedo: " << albedoName;
            return diffuse;
        }

        return materialAlebedoSearch->second;
    };

    for (auto &model : modelList)
    {
        auto modelName(model.name);
        for (auto replacement : {"$", "<", ">"})
        {
            String::Replace(modelName, replacement, "");
        }

        auto outputParentPath(FileSystem::CombinePaths(dataPath, "models", sourceName.withoutExtension().getString()));
        auto outputPath(FileSystem::CombinePaths(outputParentPath, modelName).withExtension(".gek"));
        LockedWrite{ std::cout } << ">     " << model.name << ": " << outputPath.getString();
        LockedWrite{ std::cout } << "      Num. Meshes: " << model.meshList.size();
        LockedWrite{ std::cout } << "      Size: Minimum[" << model.boundingBox.minimum.x << ", " << model.boundingBox.minimum.y << ", " << model.boundingBox.minimum.z << "]";
        LockedWrite{ std::cout } << "      Size: Maximum[" << model.boundingBox.maximum.x << ", " << model.boundingBox.maximum.y << ", " << model.boundingBox.maximum.z << "]";
        for (auto &mesh : model.meshList)
        {
            mesh.material = findMaterialForDiffuse(mesh.diffuse);
        }
        
        outputParentPath.createChain();
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

        for (auto &mesh : model.meshList)
        {
            LockedWrite{ std::cout } << "-    Mesh: " << mesh.material;
            LockedWrite{ std::cout } << "        Num. Vertices: " << mesh.pointList.size();
            LockedWrite{ std::cout } << "        Num. Faces: " << mesh.faceList.size();

            Header::Mesh meshHeader;
            std::strncpy(meshHeader.material, mesh.material.data(), 63);
            meshHeader.vertexCount = mesh.pointList.size();
            meshHeader.faceCount = mesh.faceList.size();
            fwrite(&meshHeader, sizeof(Header::Mesh), 1, file);
        }

        for (auto &mesh : model.meshList)
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

    return 0;
}
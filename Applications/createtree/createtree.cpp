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
    std::string diffuse;
    std::string material;
    std::vector<uint16_t> indexList;
    std::vector<Math::Float3> vertexList;
};

struct Model
{
    Shapes::AlignedBox boundingBox;
    std::vector<Mesh> meshList;
};

struct Parameters
{
    float feetPerUnit = 1.0f;
};

void GetNodeMeshes(const Parameters &parameters, const aiScene *scene, const aiNode *inputNode, Model &model)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << "Invalid scene inputNode";
        return;
    }

    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid mesh list";
        }
        else
        {
            for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
            {
                uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
                if (nodeMeshIndex >= scene->mNumMeshes)
                {
                    LockedWrite{ std::cerr } << "Invalid mesh index";
                }

                const aiMesh *inputMesh = scene->mMeshes[nodeMeshIndex];
                if (inputMesh->mNumFaces > 0)
                {
                    if (inputMesh->mFaces == nullptr)
                    {
                        LockedWrite{ std::cerr } << "Invalid mesh face list";
                    }

                    if (inputMesh->mVertices == nullptr)
                    {
                        LockedWrite{ std::cerr } << "Invalid mesh vertex list";
                    }

                    Mesh mesh;
                    for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                    {
                        const aiFace &face = inputMesh->mFaces[faceIndex];
                        if (face.mNumIndices == 3)
                        {
                            mesh.indexList.push_back(face.mIndices[0]);
                            mesh.indexList.push_back(face.mIndices[1]);
                            mesh.indexList.push_back(face.mIndices[2]);
                        }
                        else
                        {
                            LockedWrite{ std::cerr } << "! (Mesh " << meshIndex << ") Invalid Face Found: " << faceIndex << " (" << face.mNumIndices << " vertices)";
                        }
                    }

                    for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                    {
                        Math::Float3 position(
                            inputMesh->mVertices[vertexIndex].x,
                            inputMesh->mVertices[vertexIndex].y,
                            inputMesh->mVertices[vertexIndex].z);
                        position *= parameters.feetPerUnit;
                        model.boundingBox.extend(position);

                        mesh.vertexList.push_back(position);
                    }

                    aiString sceneDiffuseMaterial;
                    const aiMaterial *sceneMaterial = scene->mMaterials[inputMesh->mMaterialIndex];
                    sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                    mesh.diffuse = sceneDiffuseMaterial.C_Str();
                    model.meshList.push_back(mesh);
                }
            }
        }
    }

    if (inputNode->mNumChildren > 0)
    {
        if (inputNode->mChildren == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid child list";
            return;
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            GetNodeMeshes(parameters, scene, inputNode->mChildren[childIndex], model);
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
    LockedWrite{ std::cout } << "GEK Part Converter";

    FileSystem::Path fileNameInput;
    FileSystem::Path fileNameOutput;
	Parameters parameters;
    bool flipWinding = false;
    for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
    {
		std::string argument(String::Narrow(argumentList[argumentIndex]));
        std::vector<std::string> arguments(String::Split(String::GetLower(argument), ':'));
        if (arguments.empty())
        {
            LockedWrite{ std::cerr } << "No arguments specified for command line parameter";
        }

        if (arguments[0] == "-input" && ++argumentIndex < argumentCount)
        {
            fileNameInput = String::Narrow(argumentList[argumentIndex]);
        }
        else if (arguments[0] == "-output" && ++argumentIndex < argumentCount)
        {
            fileNameOutput = String::Narrow(argumentList[argumentIndex]);
        }
        else if (arguments[0] == "-flipwinding")
        {
            flipWinding = true;
        }
		else if (arguments[0] == "-unitsinfoot")
		{
			if (arguments.size() != 2)
			{
				LockedWrite{ std::cerr } << "Missing parameters for unitsInFoot";
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
        aiComponent_TEXCOORDS |
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

    unsigned int postProcessFlags =
        aiProcess_JoinIdenticalVertices |
        aiProcess_FindInvalidData |
        aiProcess_OptimizeGraph |
        0;

    aiPropertyStore *propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
    auto scene = aiImportFileExWithProperties(fileNameInput.getString().data(), importFlags, nullptr, propertyStore);
    if (scene == nullptr)
    {
        LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
    }

    scene = aiApplyPostProcessing(scene, postProcessFlags);
    if (scene == nullptr)
	{
		LockedWrite{ std::cerr } << "Unable to apply post processing with Assimp";
	}

    if (!scene->HasMeshes())
    {
        LockedWrite{ std::cerr } << "Scene has no meshes";
    }

    if (!scene->HasMaterials())
    {
        LockedWrite{ std::cerr } << "Exporting to tree requires materials in scene";
    }

    Model model;
    GetNodeMeshes(parameters, scene, scene->mRootNode, model);
    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(scene);

    auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
    auto dataPath(FileSystem::CombinePaths(rootPath, "Data"));

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
            JSON::Instance materialNode = JSON::Load(filePath);
            auto shaderNode = materialNode.get("shader");
            auto dataNode = shaderNode.get("data");
            auto albedoNode = dataNode.get("albedo");
            std::string albedoPath(albedoNode.get("file").convert(String::Empty));
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
    }

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
        mesh.material = findMaterialForDiffuse(mesh.diffuse);
        materialList.insert(mesh.material);
    }

    NewtonTreeCollisionBeginBuild(newtonCollision);
    for (auto const &mesh : model.meshList)
    {
		LockedWrite{ std::cout } << "-   " << mesh.material;
        LockedWrite{ std::cout } << "    " << mesh.vertexList.size() << "  vertices";
        LockedWrite{ std::cout } << "    " << mesh.indexList.size() << " indices";

        auto materialSearch = materialList.find(mesh.material);
        auto materialIndex = std::distance(std::begin(materialList), materialSearch);

        auto &indexList = mesh.indexList;
        auto &vertexList = mesh.vertexList;
        for (uint32_t index = 0; index < indexList.size(); index += 3)
        {
            Math::Float3 faceList[3] =
            {
                vertexList[indexList[index + 0]],
                vertexList[indexList[index + 1]],
                vertexList[indexList[index + 2]],
            };

            NewtonTreeCollisionAddFace(newtonCollision, 3, faceList[0].data, sizeof(Math::Float3), materialIndex);
        }
    }

    NewtonTreeCollisionEndBuild(newtonCollision, 1);

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput.getWindowsString().data(), L"wb");
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
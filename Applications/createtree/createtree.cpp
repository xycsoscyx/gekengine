#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Exceptions.hpp"
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

void getSceneParts(const Parameters &parameters, const aiScene *scene, const aiNode *inputNode, Model &model)
{
    if (inputNode == nullptr)
    {
        throw std::exception("Invalid scene inputNode");
    }

    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            throw std::exception("Invalid mesh list");
        }

        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                throw std::exception("Invalid mesh index");
            }

            const aiMesh *inputMesh = scene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    throw std::exception("Invalid mesh face list");
                }

				if (inputMesh->mVertices == nullptr)
				{
					throw std::exception("Invalid mesh vertex list");
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
						LockedWrite{ std::cerr } << String::Format("! (Mesh %v) Invalid Face Found: %v (%v vertices)", meshIndex, faceIndex, face.mNumIndices);
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

    if (inputNode->mNumChildren > 0)
    {
        if (inputNode->mChildren == nullptr)
        {
            throw std::exception("Invalid child list");
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            getSceneParts(parameters, scene, inputNode->mChildren[childIndex], model);
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
	try
    {
        LockedWrite{ std::cout } << String::Format("GEK Part Converter");

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
                throw std::exception("No arguments specified for command line parameter");
            }

            if (arguments[0] == "-input" && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0] == "-output" && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0] == "-flipwinding")
            {
                flipWinding = true;
            }
			else if (arguments[0] == "-unitsinfoot")
			{
				if (arguments.size() != 2)
				{
					throw std::exception("Missing parameters for unitsInFoot");
				}

				parameters.feetPerUnit = (1.0f / String::Convert(arguments[1], 1.0f));
			}
		}

		aiLogStream logStream;
		logStream.callback = [](char const *message, char *user) -> void
		{
			LockedWrite{ std::cerr } << String::Format("Assimp: %v", message);
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
        auto scene = aiImportFileExWithProperties(fileNameInput.u8string().c_str(), importFlags, nullptr, propertyStore);
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
            throw std::exception("Exporting to tree requires materials in scene");
        }

        Model model;
        getSceneParts(parameters, scene, scene->mRootNode, model);
        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, "Data"));

		std::string texturesPath(String::GetLower(FileSystem::GetFileName(dataPath, "Textures").u8string()));
        auto materialsPath(FileSystem::GetFileName(dataPath, "Materials").u8string());

        std::map<std::string, std::string> diffuseToMaterialMap;
        std::function<bool(FileSystem::Path const &)> findMaterials;
        findMaterials = [&](FileSystem::Path const &filePath) -> bool
        {
            if (filePath.isDirectory())
            {
                FileSystem::Find(filePath, findMaterials);
            }
            else if (filePath.isFile())
            {
                JSON::Instance materialNode = JSON::Load(filePath);
                auto shaderNode = materialNode.get("shader");
                auto dataNode = shaderNode.get("data");
                auto albedoNode = dataNode.get("albedo");
                std::string albedoPath(albedoNode.get("file").convert(String::Empty));
                std::string materialName(String::GetLower(filePath.withoutExtension().u8string().substr(materialsPath.size() + 1)));
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

        FileSystem::Find(materialsPath, findMaterials);
        if (diffuseToMaterialMap.empty())
        {
            throw std::exception("Unable to locate any materials");
        }

        auto findMaterialForDiffuse = [&](std::string const &diffuse) -> std::string
        {
            FileSystem::Path diffusePath(diffuse);
            std::string albedoName(String::GetLower(diffusePath.withoutExtension().u8string()));
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
                LockedWrite{ std::cerr } << String::Format("! Unable to find material for albedo: %v", albedoName.c_str());
                return diffuse;
            }

            return materialAlebedoSearch->second;
        };

		LockedWrite{ std::cout } << String::Format("> Num. Meshes: %v", model.meshList.size());
        LockedWrite{ std::cout } << String::Format("< Size: Minimum[%v, %v, %v]", model.boundingBox.minimum.x, model.boundingBox.minimum.y, model.boundingBox.minimum.z);
        LockedWrite{ std::cout } << String::Format("< Size: Maximum[%v, %v, %v]", model.boundingBox.maximum.x, model.boundingBox.maximum.y, model.boundingBox.maximum.z);

        NewtonWorld *newtonWorld = NewtonCreate();
        NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
        if (newtonCollision == nullptr)
        {
            throw std::exception("Unable to create tree collision object");
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
			LockedWrite{ std::cout } << String::Format("-  %v", mesh.material);
            LockedWrite{ std::cout } << String::Format("    %v vertices", mesh.vertexList.size());
            LockedWrite{ std::cout } << String::Format("    %v indices", mesh.indexList.size());

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
        _wfopen_s(&file, fileNameOutput.c_str(), L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.materialCount = materialList.size();
        fwrite(&header, sizeof(Header), 1, file);
        for (auto const &material : materialList)
        {
            Header::Material materialHeader;
            std::strncpy(materialHeader.name, material.c_str(), 63);
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
        fclose(file);

        NewtonDestroyCollision(newtonCollision);
        NewtonDestroy(newtonWorld);
    }
    catch (const std::exception &exception)
    {
		LockedWrite{ std::cerr } << String::Format("GEK Engine - Error");
		LockedWrite{ std::cerr } << String::Format("Caught: %v", exception.what());
		LockedWrite{ std::cerr } << String::Format("Type: %v", typeid(exception).name());
	}
    catch (...)
    {
        LockedWrite{ std::cerr } << String::Format("GEK Engine - Error");
        LockedWrite{ std::cerr } << String::Format("Caught: Non-standard exception");
    };

    return 0;
}
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

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

    uint32_t partCount = 0;
};

struct Part
{
    std::vector<uint16_t> indexList;
    std::vector<Math::Float3> vertexList;
};

struct Parameters
{
    float feetPerUnit = 1.0f;
};

void getSceneParts(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::unordered_map<std::string, std::vector<Part>> &scenePartMap, Shapes::AlignedBox &boundingBox)
{
    if (node == nullptr)
    {
        throw std::exception("Invalid scene node");
    }

    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            throw std::exception("Invalid mesh list");
        }

        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                throw std::exception("Invalid mesh index");
            }

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                if (mesh->mFaces == nullptr)
                {
                    throw std::exception("Invalid mesh face list");
                }

				if (mesh->mVertices == nullptr)
				{
					throw std::exception("Invalid mesh vertex list");
				}

                Part part;
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = mesh->mFaces[faceIndex];
                    if (face.mNumIndices == 3)
                    {
						part.indexList.push_back(face.mIndices[0]);
                        part.indexList.push_back(face.mIndices[1]);
						part.indexList.push_back(face.mIndices[2]);
                    }
                    else
                    {
						std::cerr << "! (Mesh " << meshIndex << ") Invalid Face Found: " << faceIndex << " (" << face.mNumIndices << " vertices)" << std::endl;
                    }
                }

                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                {
                    Math::Float3 position(
                        mesh->mVertices[vertexIndex].x,
                        mesh->mVertices[vertexIndex].y,
                        mesh->mVertices[vertexIndex].z);
                    position *= parameters.feetPerUnit;
                    boundingBox.extend(position);

                    part.vertexList.push_back(position);
                }

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string materialPath(sceneDiffuseMaterial.C_Str());
                scenePartMap[materialPath].push_back(part);
            }
        }
    }

    if (node->mNumChildren > 0)
    {
        if (node->mChildren == nullptr)
        {
            throw std::exception("Invalid child list");
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            getSceneParts(parameters, scene, node->mChildren[childIndex], scenePartMap, boundingBox);
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
        std::cout << "GEK Part Converter" << std::endl;

        std::string fileNameInput;
        std::string fileNameOutput;
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

            if (arguments[0] == "-input"s && ++argumentIndex < argumentCount)
            {
                fileNameInput = String::Narrow(argumentList[argumentIndex]);
            }
            else if (arguments[0] == "-output"s && ++argumentIndex < argumentCount)
            {
                fileNameOutput = String::Narrow(argumentList[argumentIndex]);
            }
            else if (arguments[0] == "-flipwinding"s)
            {
                flipWinding = true;
            }
			else if (arguments[0] == "-unitsinfoot"s)
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
			std::cerr << "Assimp: " << message;
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
        auto scene = aiImportFileExWithProperties(fileNameInput.c_str(), importFlags, nullptr, propertyStore);
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

		Shapes::AlignedBox boundingBox;
        std::unordered_map<std::string, std::vector<Part>> scenePartMap;
        getSceneParts(parameters, scene, scene->mRootNode, scenePartMap, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, "Data"s));

		std::string texturesPath(String::GetLower(FileSystem::GetFileName(dataPath, "Textures"s).u8string()));
        auto materialsPath(FileSystem::GetFileName(dataPath, "Materials"s).u8string());

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
                try
                {
                    const JSON::Object materialNode = JSON::Load(filePath);
                    auto &shaderNode = materialNode["shader"s];
                    auto &passesNode = shaderNode["passes"s];
                    auto &solidNode = passesNode["solid"s];
                    auto &dataNode = solidNode["data"s];
                    auto &albedoNode = dataNode["albedo"s];
                    if (albedoNode.is_object())
                    {
                        if (albedoNode.has_member("file"s))
                        {
							std::string materialName(String::GetLower(filePath.withoutExtension().u8string().substr(materialsPath.size() + 1)));
                            std::string albedoPath(albedoNode["file"s].as_string());
                            albedoToMaterialMap[albedoPath] = materialName;
                        }
                    }
                }
                catch (...)
                {
                };
            }

            return true;
        };

        auto engineIndex = texturesPath.find("gek engine"s);
        if (engineIndex != std::string::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.substr(engineIndex);
        }

        FileSystem::Find(materialsPath, findMaterials);
        if (albedoToMaterialMap.empty())
        {
            throw std::exception("Unable to locate any materials");
        }

        std::unordered_map<std::string, std::vector<Part>> albedoPartMap;
        for (const auto &modelAlbedo : scenePartMap)
        {
			std::string albedoName(String::GetLower(FileSystem::Path(modelAlbedo.first).withoutExtension().u8string()));
            if (albedoName.find("textures\\"s) == 0)
            {
                albedoName = albedoName.substr(9);
            }
            else if (albedoName.find("..\\textures\\"s) == 0)
            {
                albedoName = albedoName.substr(12);
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
                std::cerr << "! Unable to find material for albedo: " << albedoName << std::endl;
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
                    material.indexList.push_back(uint16_t(index + material.vertexList.size()));
                }

                material.vertexList.insert(std::end(material.vertexList), std::begin(instance.vertexList), std::end(instance.vertexList));
            }
        }

		if (materialPartMap.empty())
		{
			throw std::exception("No valid material models found");
		}

		std::cout << "> Num. Parts: " << materialPartMap.size() << std::endl;
		std::cout << "< Size: Min(" << boundingBox.minimum.x << ", " << boundingBox.minimum.y << ", " << boundingBox.minimum.z << ")" << std::endl;
		std::cout << "<       Max(" << boundingBox.maximum.x << ", " << boundingBox.maximum.y << ", " << boundingBox.maximum.z << ")" << std::endl;

        NewtonWorld *newtonWorld = NewtonCreate();
        NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
        if (newtonCollision == nullptr)
        {
            throw std::exception("Unable to create tree collision object");
        }

        int materialIdentifier = 0;
        NewtonTreeCollisionBeginBuild(newtonCollision);
        for (const auto &material : materialPartMap)
        {
			std::cout << "-  " << material.first << std::endl;
            std::cout << "    " << material.second.vertexList.size() << " vertices" << std::endl;
            std::cout << "    " << material.second.indexList.size() << " indices" << std::endl;

            auto &indexList = material.second.indexList;
            auto &vertexList = material.second.vertexList;
            for (uint32_t index = 0; index < indexList.size(); index += 3)
            {
                Math::Float3 faceList[3] =
                {
                    vertexList[indexList[index + 0]],
                    vertexList[indexList[index + 1]],
                    vertexList[indexList[index + 2]],
                };

                NewtonTreeCollisionAddFace(newtonCollision, 3, faceList[0].data, sizeof(Math::Float3), materialIdentifier);
            }

            ++materialIdentifier;
        }

        NewtonTreeCollisionEndBuild(newtonCollision, 1);

        FILE *file = nullptr;
        _wfopen_s(&file, String::Widen(fileNameOutput).c_str(), L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.partCount = materialPartMap.size();
        fwrite(&header, sizeof(Header), 1, file);
        for (const auto &material : materialPartMap)
        {
            Header::Material materialHeader;
            std::strncpy(materialHeader.name, material.first.c_str(), 63);
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
        fclose(file);

        NewtonDestroyCollision(newtonCollision);
        NewtonDestroy(newtonWorld);
    }
    catch (const std::exception &exception)
    {
		std::cerr << "GEK Engine - Error" << std::endl;
		std::cerr << "Caught: " << exception.what() << std::endl;
		std::cerr << "Type: " << typeid(exception).name() << std::endl;
	}
    catch (...)
    {
        std::cerr << "GEK Engine - Error" << std::endl;
        std::cerr << "Caught: Non-standard exception" << std::endl;
    };

    std::cout << "" << std::endl;
    return 0;
}
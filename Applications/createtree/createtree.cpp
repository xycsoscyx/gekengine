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
        wchar_t name[64] = L"";
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

void getSceneParts(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::unordered_map<FileSystem::Path, std::vector<Part>> &scenePartMap, Shapes::AlignedBox &boundingBox)
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
                        printf("(Mesh %d) Invalid Face Found: %d (%d vertices)\r\n", meshIndex, faceIndex, face.mNumIndices);
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
                FileSystem::Path materialPath(sceneDiffuseMaterial.C_Str());
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
        printf("GEK Part Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
		Parameters parameters;
        bool flipWinding = false;
        for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
        {
            String argument(argumentList[argumentIndex]);
            std::vector<String> arguments(argument.split(L':'));
            if (arguments.empty())
            {
                throw std::exception("No arguments specified for command line parameter");
            }

            if (arguments[0].compareNoCase(L"-input") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameInput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-output") == 0 && ++argumentIndex < argumentCount)
            {
                fileNameOutput = argumentList[argumentIndex];
            }
            else if (arguments[0].compareNoCase(L"-flipWinding") == 0)
            {
                flipWinding = true;
            }
			else if (arguments[0].compareNoCase(L"-unitsInFoot") == 0)
			{
				if (arguments.size() != 2)
				{
					throw std::exception("Missing parameters for unitsInFoot");
				}

				parameters.feetPerUnit = (1.0f / (float)arguments[1]);
			}
		}

		aiLogStream logStream;
		logStream.callback = [](char const *message, char *user) -> void
		{
			printf("Assimp: %s", message);
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
            aiProcess_FindInstances |
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
        auto scene = aiImportFileExWithProperties(StringUTF8(fileNameInput), importFlags, nullptr, propertyStore);
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
        std::unordered_map<FileSystem::Path, std::vector<Part>> scenePartMap;
        getSceneParts(parameters, scene, scene->mRootNode, scenePartMap, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, L"Data"));

        String texturesPath(FileSystem::GetFileName(dataPath, L"Textures"));
        texturesPath.toLower();

        String materialsPath(FileSystem::GetFileName(dataPath, L"Materials"));
        materialsPath.toLower();

        std::map<FileSystem::Path, String> pathToAlbedoMap;
        std::function<bool(const FileSystem::Path &)> findMaterials;
        findMaterials = [&](const FileSystem::Path &filePath) -> bool
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
                    auto &shaderNode = materialNode[L"shader"];
                    auto &passesNode = shaderNode[L"passes"];
                    auto &solidNode = passesNode[L"solid"];
                    auto &dataNode = solidNode[L"data"];
                    auto &albedoNode = dataNode[L"albedo"];
                    if (albedoNode.is_object())
                    {
                        if (albedoNode.has_member(L"file"))
                        {
                            String materialName(filePath.withoutExtension());
                            materialName = materialName.subString(materialsPath.size() + 1);
                            materialName.toLower();

                            FileSystem::Path albedoPath(albedoNode[L"file"].as_string());
                            pathToAlbedoMap[albedoPath] = materialName;

                            //printf("Material %S with %S albedo\r\n", materialName.c_str(), albedoPath.c_str());
                        }
                    }
                }
                catch (...)
                {
                };
            }

            return true;
        };

        auto engineIndex = texturesPath.find(L"gek engine");
        if (engineIndex != String::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.subString(engineIndex);
        }

        FileSystem::Find(materialsPath, findMaterials);
        std::unordered_map<FileSystem::Path, std::vector<Part>> albedoPartMap;
        for (auto &modelAlbedo : scenePartMap)
        {
            String albedoName(modelAlbedo.first.withoutExtension());
            albedoName.toLower();

            if (albedoName.find(L"textures\\") == 0)
            {
                albedoName = albedoName.subString(9);
            }
            else if (albedoName.find(L"..\\textures\\") == 0)
            {
                albedoName = albedoName.subString(12);
            }
            else
            {
                auto texturesIndex = albedoName.find(texturesPath);
                if (texturesIndex != String::npos)
                {
                    albedoName = albedoName.subString(texturesIndex + texturesPath.length() + 1);
                }
            }

            //printf("FileName: %S\r\n", modelAlbedo.first.c_str());
            //printf("Albedo: %S\r\n", albedoName.c_str());

            auto materialAlebedoSearch = pathToAlbedoMap.find(albedoName);
            if (materialAlebedoSearch == std::end(pathToAlbedoMap))
            {
                printf("! Unable to find material for albedo: %S\r\n", albedoName.c_str());
            }
            else
            {
                albedoPartMap[materialAlebedoSearch->second] = modelAlbedo.second;
                //printf("Remap: %S: %S\r\n", albedoName.c_str(), materialAlebedoSearch->second.c_str());
            }
        }

        std::unordered_map<FileSystem::Path, Part> materialPartMap;
        for (auto &multiMaterial : albedoPartMap)
        {
            Part &material = materialPartMap[multiMaterial.first];
            for (auto &instance : multiMaterial.second)
            {
                for (auto &index : instance.indexList)
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

		printf("> Num. Parts: %d\r\n", materialPartMap.size());
        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

        NewtonWorld *newtonWorld = NewtonCreate();
        NewtonCollision *newtonCollision = NewtonCreateTreeCollision(newtonWorld, 0);
        if (newtonCollision == nullptr)
        {
            throw std::exception("Unable to create tree collision object");
        }

        int materialIdentifier = 0;
        NewtonTreeCollisionBeginBuild(newtonCollision);
        for (auto &material : materialPartMap)
        {
            printf("-  %S\r\n", material.first.c_str());
            printf("    %d vertices\r\n", material.second.vertexList.size());
            printf("    %d indices\r\n", material.second.indexList.size());

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
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.partCount = materialPartMap.size();
        fwrite(&header, sizeof(Header), 1, file);
        for (auto &material : materialPartMap)
        {
            Header::Material materialHeader;
            wcsncpy(materialHeader.name, material.first, 63);
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
        fclose(file);

        NewtonDestroyCollision(newtonCollision);
        NewtonDestroy(newtonWorld);
    }
    catch (const std::exception &exception)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf(StringUTF8::Format("Caught: %v\r\nType: %v\r\n", exception.what(), typeid(exception).name()));
    }
    catch (...)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf("Caught: Non-standard exception\r\n");
    };

    printf("\r\n");
    return 0;
}
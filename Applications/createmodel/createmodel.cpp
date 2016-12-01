#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include <Windows.h>
#include <experimental\filesystem>
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
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 0;
    uint16_t version = 5;

    Shapes::AlignedBox boundingBox;

    uint32_t materialCount;
};

struct Vertex
{
    Math::Float3 position;
    Math::Float2 texCoord;
	Math::Float3 tangent;
	Math::Float3 biTangent;
	Math::Float3 normal;
};

struct Model
{
    std::vector<uint16_t> indexList;
    std::vector<Vertex> vertexList;
};

struct Parameters
{
    float feetPerUnit = 1.0f;
};

void getMeshes(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::unordered_map<StringUTF8, std::list<Model>> &albedoMap, Shapes::AlignedBox &boundingBox)
{
    if (node == nullptr)
    {
        throw std::exception("Invalid model node");
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

				if (mesh->mTextureCoords[0] == nullptr)
				{
					throw std::exception("Invalid mesh texture coordinate list");
				}

				if (mesh->mTangents == nullptr)
				{
					throw std::exception("Invalid mesh tangent list");
				}

				if (mesh->mBitangents == nullptr)
				{
					throw std::exception("Invalid mesh bitangent list");
				}

				if (mesh->mNormals == nullptr)
				{
					throw std::exception("Invalid mesh normal list");
				}

                Model model;
                for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = mesh->mFaces[faceIndex];
                    if (face.mNumIndices == 3)
                    {
						model.indexList.push_back(face.mIndices[0]);
                        model.indexList.push_back(face.mIndices[1]);
						model.indexList.push_back(face.mIndices[2]);
                    }
                    else
                    {
                        printf("(Mesh %d) Invalid Face Found: %d (%d vertices)\r\n", meshIndex, faceIndex, face.mNumIndices);
                    }
                }

                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                {
                    Vertex vertex;
                    vertex.position.set(
                        mesh->mVertices[vertexIndex].x,
                        mesh->mVertices[vertexIndex].y,
                        mesh->mVertices[vertexIndex].z);
                    vertex.position *= parameters.feetPerUnit;
                    boundingBox.extend(vertex.position);

					vertex.texCoord.set(
						mesh->mTextureCoords[0][vertexIndex].x,
						mesh->mTextureCoords[0][vertexIndex].y);

                    vertex.tangent.set(
						mesh->mTangents[vertexIndex].x,
						mesh->mTangents[vertexIndex].y,
						mesh->mTangents[vertexIndex].z);

                    vertex.biTangent.set(
                        mesh->mBitangents[vertexIndex].x,
						mesh->mBitangents[vertexIndex].y,
						mesh->mBitangents[vertexIndex].z);

                    vertex.normal.set(
                        mesh->mNormals[vertexIndex].x,
						mesh->mNormals[vertexIndex].y,
						mesh->mNormals[vertexIndex].z);

                    model.vertexList.push_back(vertex);
                }

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = scene->mMaterials[mesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                StringUTF8 material(sceneDiffuseMaterial.C_Str());
                albedoMap[material].push_back(model);
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
            getMeshes(parameters, scene, node->mChildren[childIndex], albedoMap, boundingBox);
        }
    }
}

void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fwrite(buffer, 1, size, file);
}

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    try
    {
        printf("GEK Model Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
		Parameters parameters;
        bool flipCoords = false;
        bool flipWinding = false;
        float smoothingAngle = 80.0f;
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
            else if (arguments[0].compareNoCase(L"-flipCoords") == 0)
            {
                flipCoords = true;
            }
            else if (arguments[0].compareNoCase(L"-flipWinding") == 0)
            {
                flipWinding = true;
            }
            else if (arguments[0].compareNoCase(L"-smoothAngle") == 0)
            {
                if (arguments.size() != 2)
                {
                    throw std::exception("Missing parameters for smoothAngle");
                }

                smoothingAngle = arguments[1];
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
		logStream.callback = [](const char *message, char *user) -> void
		{
			printf("Assimp: %s", message);
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
            aiProcess_OptimizeGraph |
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
            aiProcess_GenUVCoords |
            aiProcess_TransformUVCoords |
            0;

        unsigned int postProcessFlags =
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            0;

        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
		aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
		aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
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
            throw std::exception("Exporting to model requires materials in scene");
        }

		Shapes::AlignedBox boundingBox;
        std::unordered_map<StringUTF8, std::list<Model>> albedoMap;
        getMeshes(parameters, scene, scene->mRootNode, albedoMap, boundingBox);

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

        String rootPath;
        String currentModuleName((MAX_PATH + 1), L' ');
        GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
        currentModuleName.trimRight();

        String fullModuleName((MAX_PATH + 1), L' ');
        GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
        fullModuleName.trimRight();

        std::experimental::filesystem::path fullModulePath(fullModuleName);
        fullModulePath.remove_filename();
        fullModulePath.remove_filename();
        rootPath = fullModulePath.append(L"Data").wstring();

        String texturesPath(FileSystem::GetFileName(rootPath, L"Textures").getLower());
        String materialsPath(FileSystem::GetFileName(rootPath, L"Materials").getLower());

        std::map<String, String> materialAlbedoMap;

        std::function<bool(const wchar_t *)> findMaterials;
        findMaterials = [&](const wchar_t *fileName) -> bool
        {
            if (FileSystem::IsDirectory(fileName))
            {
                FileSystem::Find(fileName, findMaterials);
            }
            else if (FileSystem::IsFile(fileName))
            {
                try
                {
                    String materialName(FileSystem::ReplaceExtension(fileName).getLower());
                    materialName.replace((materialsPath + L"\\"), L"");

                    const JSON::Object materialNode = JSON::Load(fileName);
                    auto &shaderNode = materialNode[L"shader"];
                    auto &passesNode = shaderNode[L"passes"];
                    auto &solidNode = passesNode[L"solid"];
                    auto &dataNode = solidNode[L"data"];
                    auto &albedoNode = dataNode[L"albedo"];
                    if (albedoNode.is_object())
                    {
                        if (albedoNode.has_member(L"file"))
                        {
                            materialAlbedoMap[albedoNode[L"file"].as_string()] = materialName;
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
        std::unordered_map<String, std::list<Model>> materialMultiMap;
        for (auto &albedo : albedoMap)
        {
            String albedoName(albedo.first.getLower());
            albedoName.replace(L"/", L"\\");
            albedoName = FileSystem::ReplaceExtension(albedoName);
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

            //printf("FileName: %s\r\n", albedo.first.c_str());
            //printf("Albedo: %S\r\n", albedoName.c_str());

            auto materialAlebedoSearch = materialAlbedoMap.find(albedoName);
            if (materialAlebedoSearch == std::end(materialAlbedoMap))
            {
                printf("! Unable to find material for albedo: %S\r\n", albedoName.c_str());
            }
            else
            {
                materialMultiMap[materialAlebedoSearch->second] = albedo.second;
                //printf("Remap: %S: %S\r\n", albedoName.c_str(), materialAlebedoSearch->second.c_str());
            }
        }

        std::unordered_map<String, Model> materialMap;
        for (auto &multiMaterial : materialMultiMap)
        {
            Model &material = materialMap[multiMaterial.first];
            for (auto &instance : multiMaterial.second)
            {
                for (auto &index : instance.indexList)
                {
                    material.indexList.push_back(uint16_t(index + material.vertexList.size()));
                }

                material.vertexList.insert(std::end(material.vertexList), std::begin(instance.vertexList), std::end(instance.vertexList));
            }
        }

		if (materialMap.empty())
		{
			throw std::exception("No valid material models found");
		}

		printf("> Num. Models: %d\r\n", materialMap.size());
        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

        FILE *file = nullptr;
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        header.materialCount = materialMap.size();
        header.boundingBox = boundingBox;
        fwrite(&header, sizeof(Header), 1, file);
        for (auto &material : materialMap)
        {
            printf("-  %S\r\n", material.first.c_str());
            printf("    %d vertices\r\n", material.second.vertexList.size());
            printf("    %d indices\r\n", material.second.indexList.size());

            Header::Material materialHeader;
            wcsncpy(materialHeader.name, material.first, 63);
            materialHeader.vertexCount = material.second.vertexList.size();
            materialHeader.indexCount = material.second.indexList.size();
            fwrite(&materialHeader, sizeof(Header::Material), 1, file);
        }

        for (auto &material : materialMap)
        {
            fwrite(material.second.indexList.data(), sizeof(uint16_t), material.second.indexList.size(), file);
            fwrite(material.second.vertexList.data(), sizeof(Vertex), material.second.vertexList.size(), file);
        }

        fclose(file);
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
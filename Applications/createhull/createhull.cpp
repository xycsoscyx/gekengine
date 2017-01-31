#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include <algorithm>
#include <vector>

#include <Newton.h>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Gek;

struct Header
{
    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 1;
    uint16_t version = 2;
    uint32_t newtonVersion = NewtonWorldGetVersion();
};

struct Parameters
{
    float feetPerUnit = 1.0f;
};

void getMeshes(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::vector<Math::Float3> &pointList, Shapes::AlignedBox &boundingBox)
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

                for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
                {
                    Math::Float3 position(
                        mesh->mVertices[vertexIndex].x,
                        mesh->mVertices[vertexIndex].y,
                        mesh->mVertices[vertexIndex].z);
                    position *= parameters.feetPerUnit;
                    boundingBox.extend(position);

                    pointList.push_back(position);
                }
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
            getMeshes(parameters, scene, node->mChildren[childIndex], pointList, boundingBox);
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
        printf("GEK Model Converter\r\n");

        String fileNameInput;
        String fileNameOutput;
		Parameters parameters;
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
            aiComponent_TEXTURES |
            aiComponent_MATERIALS |
            0;

        unsigned int importFlags =
            aiProcess_RemoveComponent |
            aiProcess_OptimizeMeshes |
            aiProcess_PreTransformVertices |
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

		Shapes::AlignedBox boundingBox;
        std::vector<Math::Float3> pointList;
        getMeshes(parameters, scene, scene->mRootNode, pointList, boundingBox);
        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(scene);

		if (pointList.empty())
		{
            throw std::exception("No vertex data found in scene");
		}

		printf("> Num. Points: %d\r\n", pointList.size());
        printf("< Size: Min(%f, %f, %f)\r\n", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
        printf("        Max(%f, %f, %f)\r\n", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

        NewtonWorld *newtonWorld = NewtonCreate();
        NewtonCollision *newtonCollision = NewtonCreateConvexHull(newtonWorld, pointList.size(), pointList.data()->data, sizeof(Math::Float3), 0.025f, 0, Math::Float4x4::Identity.data);
        if (newtonCollision == nullptr)
        {
            throw std::exception("Unable to create convex hull collision object");
        }

        FILE *file = nullptr;
        _wfopen_s(&file, fileNameOutput, L"wb");
        if (file == nullptr)
        {
            throw std::exception("Unable to create output file");
        }

        Header header;
        fwrite(&header, sizeof(Header), 1, file);
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
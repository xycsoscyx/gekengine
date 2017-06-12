#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Context.hpp"
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

bool getMeshes(const Parameters &parameters, const aiScene *scene, const aiNode *node, std::vector<Math::Float3> &pointList, Shapes::AlignedBox &boundingBox)
{
    if (node == nullptr)
    {
        LockedWrite{std::cerr} << String::Format("Invalid model node");
        return false;
    }

    if (node->mNumMeshes > 0)
    {
        if (node->mMeshes == nullptr)
        {
            LockedWrite{std::cerr} << String::Format("Invalid mesh list");
            return false;
        }

        for (uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = node->mMeshes[meshIndex];
            if (nodeMeshIndex >= scene->mNumMeshes)
            {
                LockedWrite{std::cerr} << String::Format("Invalid mesh index");
                return false;
            }

            const aiMesh *mesh = scene->mMeshes[nodeMeshIndex];
            if (mesh->mNumFaces > 0)
            {
                if (mesh->mFaces == nullptr)
                {
                    LockedWrite{std::cerr} << String::Format("Invalid mesh face list");
                    return false;
                }

				if (mesh->mVertices == nullptr)
				{
					LockedWrite{std::cerr} << String::Format("Invalid mesh vertex list");
                    return false;
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
            LockedWrite{std::cerr} << String::Format("Invalid child list");
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        {
            if (!getMeshes(parameters, scene, node->mChildren[childIndex], pointList, boundingBox))
            {
                return false;
            }
        }
    }

    return true;
}

void serializeCollision(void* const serializeHandle, const void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fwrite(buffer, 1, size, file);
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{std::cout} << String::Format("GEK Model Converter");

    FileSystem::Path fileNameInput;
    FileSystem::Path fileNameOutput;
	Parameters parameters;
    for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
    {
		std::string argument(String::Narrow(argumentList[argumentIndex]));
		std::vector<std::string> arguments(String::Split(String::GetLower(argument), ':'));
        if (arguments.empty())
        {
            LockedWrite{std::cerr} << String::Format("No arguments specified for command line parameter");
            return -__LINE__;
        }

        if (arguments[0] == "-input" && ++argumentIndex < argumentCount)
        {
            fileNameInput = argumentList[argumentIndex];
        }
        else if (arguments[0] == "-output" && ++argumentIndex < argumentCount)
        {
            fileNameOutput = argumentList[argumentIndex];
        }
		else if (arguments[0] == "-unitsinfoot")
		{
			if (arguments.size() != 2)
			{
				LockedWrite{std::cerr} << String::Format("Missing parameters for unitsInFoot");
                return -__LINE__;
            }

			parameters.feetPerUnit = (1.0f / (float)String::Convert(arguments[1], 1.0f));
		}
	}

	aiLogStream logStream;
	logStream.callback = [](char const *message, char *user) -> void
	{
		LockedWrite{std::cerr} << String::Format("Assimp: %v", message);
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
    auto scene = aiImportFileExWithProperties(fileNameInput.u8string().c_str(), importFlags, nullptr, propertyStore);
    if (scene == nullptr)
    {
        LockedWrite{std::cerr} << String::Format("Unable to load scene with Assimp");
        return -__LINE__;
    }

    scene = aiApplyPostProcessing(scene, postProcessFlags);
    if (scene == nullptr)
	{
		LockedWrite{std::cerr} << String::Format("Unable to apply post processing with Assimp");
        return -__LINE__;
    }

    if (!scene->HasMeshes())
    {
        LockedWrite{std::cerr} << String::Format("Scene has no meshes");
        return -__LINE__;
    }

	Shapes::AlignedBox boundingBox;
    std::vector<Math::Float3> pointList;
    if (!getMeshes(parameters, scene, scene->mRootNode, pointList, boundingBox))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(scene);

	if (pointList.empty())
	{
        LockedWrite{std::cerr} << String::Format("No vertex data found in scene");
        return -__LINE__;
    }

	LockedWrite{std::cout} << String::Format("> Num. Points: %v", pointList.size());
    LockedWrite{std::cout} << String::Format("< Size: Minimum[%v, %v, %v]", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
    LockedWrite{std::cout} << String::Format("< Size: Maximum[%v, %v, %v]", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

    NewtonWorld *newtonWorld = NewtonCreate();
    NewtonCollision *newtonCollision = NewtonCreateConvexHull(newtonWorld, pointList.size(), pointList.data()->data, sizeof(Math::Float3), 0.025f, 0, Math::Float4x4::Identity.data);
    if (newtonCollision == nullptr)
    {
        LockedWrite{std::cerr} << String::Format("Unable to create convex hull collision object");
        return -__LINE__;
    }

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput.c_str(), L"wb");
    if (file == nullptr)
    {
        LockedWrite{std::cerr} << String::Format("Unable to create output file");
        return -__LINE__;
    }

    Header header;
    fwrite(&header, sizeof(Header), 1, file);
    NewtonCollisionSerialize(newtonWorld, newtonCollision, serializeCollision, file);
    fclose(file);

    NewtonDestroyCollision(newtonCollision);
    NewtonDestroy(newtonWorld);

    return 0;
}
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>
#include <vector>

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace Gek;

struct Header
{
    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 1;
    uint16_t version = 3;
};

struct Parameters
{
    std::string sourceName;
    std::string targetName;
    float feetPerUnit = 1.0f;
};

bool GetModels(Parameters const& parameters, aiScene const* inputScene, aiNode const* inputNode, aiMatrix4x4 const& parentTransform, std::vector<Math::Float3> &pointList, Shapes::AlignedBox &boundingBox)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << "Invalid scene node";
        return false;
    }

    aiMatrix4x4 transform(parentTransform * inputNode->mTransformation);
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            LockedWrite{ std::cerr } << "Invalid mesh list";
            return false;
        }

        std::string name = inputNode->mName.C_Str();
        LockedWrite{ std::cout } << "Found Assimp Model: " << name;
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                LockedWrite{ std::cerr } << "Invalid mesh index";
                continue;
            }

            const aiMesh* inputMesh = inputScene->mMeshes[nodeMeshIndex];
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

                pointList.reserve(pointList.size() + inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertex = inputMesh->mVertices[vertexIndex];
                    aiTransformVecByMatrix4(&vertex, &transform);
                    vertex *= parameters.feetPerUnit;
                    pointList.push_back(Math::Float3(vertex.x, vertex.y, vertex.z));
                    boundingBox.extend(Math::Float3(vertex.x, vertex.y, vertex.z));
                }
            }
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
            if (!GetModels(parameters, inputScene, inputNode->mChildren[childIndex], transform, pointList, boundingBox))
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
    LockedWrite{ std::cout } << "GEK Hull Converter";

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
        else if (arguments[0] == "-target" && ++argumentIndex < argumentCount)
        {
            parameters.targetName = String::Narrow(argumentList[argumentIndex]);
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
    logStream.callback = [](char const* message, char* user) -> void
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

    static const unsigned int importFlags =
        aiProcess_RemoveComponent |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_ValidateDataStructure;

    aiPropertyStore* propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);

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

        auto filePath = context->findDataPath(FileSystem::CreatePath("physics", parameters.sourceName));

        LockedWrite{ std::cout } << "Loading: " << filePath.getString();
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
            return -__LINE__;
        }

        static const unsigned int postProcessSteps[] =
        {
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            0,

            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph,
            0,
        };

        for (auto postProcessFlags : postProcessSteps)
        {
            inputScene = aiApplyPostProcessing(inputScene, postProcessFlags);
            if (inputScene == nullptr)
            {
                LockedWrite{ std::cerr } << "Unable to apply post processing with Assimp";
                return -__LINE__;
            }
        }

        if (!inputScene->HasMeshes())
        {
            LockedWrite{ std::cerr } << "Scene has no meshes";
            return -__LINE__;
        }

        aiMatrix4x4 identity;
        Shapes::AlignedBox boundingBox;
        std::vector<Math::Float3> pointList;
        if (!GetModels(parameters, inputScene, inputScene->mRootNode, identity, pointList, boundingBox))
        {
            return -__LINE__;
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(inputScene);

        if (pointList.empty())
        {
            LockedWrite{ std::cerr } << "No vertex data found in scene";
            return -__LINE__;
        }

        LockedWrite{ std::cout } << "> Num. Points: " << pointList.size();
        LockedWrite{ std::cout } << "< Size: Minimum[" << boundingBox.minimum.x << ", " << boundingBox.minimum.y << ", " << boundingBox.minimum.z << "]";
        LockedWrite{ std::cout } << "< Size: Maximum[" << boundingBox.maximum.x << ", " << boundingBox.maximum.y << ", " << boundingBox.maximum.z << "]";

        auto outputPath(filePath.withoutExtension().withExtension(".gek"));
        LockedWrite{ std::cout } << "Writing: " << outputPath.getString();
        outputPath.getParentPath().createChain();

        FILE* file = nullptr;
        _wfopen_s(&file, outputPath.getWideString().data(), L"wb");
        if (file == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to create output file";
            return -__LINE__;
        }

        Header header;
        fwrite(&header, sizeof(Header), 1, file);

        uint32_t pointCount = pointList.size();
        fwrite(&pointCount, sizeof(uint32_t), 1, file);
        fwrite(pointList[0].data, sizeof(Math::Float3), pointCount, file);

        fclose(file);
    }

    return 0;
}
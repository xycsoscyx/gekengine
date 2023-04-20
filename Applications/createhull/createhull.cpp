#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>
#include <vector>

#include <argparse/argparse.hpp>
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
    float feetPerUnit;
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
    LockedWrite{ std::cout } << "GEK Convex Hull Converter";

    argparse::ArgumentParser program("GEK Convex Hull Converter", "1.0");

    program.add_argument("-i", "--input")
        .required()
        .help("input model");

    program.add_argument("-o", "--output")
        .required()
        .help("output model");

    program.add_argument("-u", "--unitsperfoot")
        .scan<'g', float>()
        .help("units per foot")
        .default_value(1.0f);

    program.add_description("Convert input model in to GEK Engine format.");
    program.add_epilog("Input model formats include anything supported by the Assimp library.");

    try
    {
        std::vector<std::string> arguments;
        for (int argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++)
        {
            arguments.push_back(String::Narrow(argumentList[argumentIndex]));
        }

        program.parse_args(arguments);
    }
    catch (const std::runtime_error& err)
    {
        LockedWrite{ std::cerr } << err.what() << std::endl;
        LockedWrite{ std::cerr } << program;
        return 1;
    }

    Parameters parameters;
    parameters.sourceName = program.get<std::string>("--input");
    parameters.targetName = program.get<std::string>("--output");
    parameters.feetPerUnit = (1.0f / program.get<float>("--unitsperfoot"));

    aiLogStream logStream;
    logStream.callback = [](char const* message, char* user) -> void
    {
        std::string trimmedMessage(message);
        trimmedMessage = trimmedMessage.substr(0, trimmedMessage.size() - 1);
        LockedWrite{ std::cerr } << "Assimp: " << trimmedMessage;
    };

    logStream.user = nullptr;
    aiAttachLogStream(&logStream);

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
            aiProcess_FindDegenerates |
            aiProcess_ValidateDataStructure |
            aiProcess_MakeLeftHanded |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            //aiProcess_OptimizeGraph,
            0;

        aiPropertyStore* propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        //aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        //aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        //aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        LockedWrite{ std::cout } << "Loading: " << filePath.getString();
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
        if (inputScene == nullptr)
        {
            LockedWrite{ std::cerr } << "Unable to load scene with Assimp";
            return -__LINE__;
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
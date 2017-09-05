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

namespace std
{
    template <class _Arg, class _Result>
    struct unary_function
    {
        typedef _Arg    argument_type;
        typedef _Result result_type;
    };
}; // namespace std

#include <assimp/config.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <OpenMesh/Core/System/config.h>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

using namespace Gek;

struct Traits : OpenMesh::DefaultTraits
{
    VertexTraits
    {
    public:
        OpenMesh::Vec3f tangent;
        OpenMesh::Vec3f biTangent;

    public:
        VertexT()
            : tangent(OpenMesh::Vec3f(0.0f, 0.0f, 0.0f))
            , biTangent(OpenMesh::Vec3f(0.0f, 0.0f, 0.0f))
        {
        }
    };
};

struct Header
{
    struct Material
    {
        char name[64] = "";
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 0;
    uint16_t version = 6;

    Shapes::AlignedBox boundingBox;

    uint32_t meshCount;
};

using Mesh = OpenMesh::TriMesh_ArrayKernelT<Traits>;

struct Parameters
{
    float feetPerUnit = 1.0f;
    std::string forceMaterial;
};

bool getSceneMeshes(const Parameters &parameters, const aiScene *inputScene, const aiNode *inputNode, std::unordered_map<std::string, Mesh> &sceneMeshMap, Shapes::AlignedBox &boundingBox)
{
    if (inputNode == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Invalid inputScene inputNode");
        return false;
    }

    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            LockedWrite{ std::cerr } << String::Format("Invalid mesh list");
            return false;
        }

        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                LockedWrite{ std::cerr } << String::Format("Invalid mesh index");
                return false;
            }

            const aiMesh *inputMesh = inputScene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh face list");
                    return false;
                }

                if (inputMesh->mVertices == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh vertex list");
                    return false;
                }

                if (inputMesh->mTextureCoords[0] == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh texture coordinate list");
                    return false;
                }

                if (inputMesh->mTangents == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh tangent list");
                    return false;
                }

                if (inputMesh->mBitangents == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh bitangent list");
                    return false;
                }

                if (inputMesh->mNormals == nullptr)
                {
                    LockedWrite{ std::cerr } << String::Format("Invalid inputMesh normal list");
                    return false;
                }

                std::string materialName;
                if (parameters.forceMaterial.empty())
                {
                    aiString sceneDiffuseMaterial;
                    const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                    sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                    materialName = sceneDiffuseMaterial.C_Str();
                }
                else
                {
                    materialName = parameters.forceMaterial;
                }

                auto &mesh = sceneMeshMap[materialName];
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertexHandle = mesh.add_vertex(OpenMesh::Vec3f(
                        (inputMesh->mVertices[vertexIndex].x * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].y * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].z * parameters.feetPerUnit)));
                    boundingBox.extend(*(Math::Float3 *)&mesh.point(vertexHandle));

                    mesh.set_texcoord2D(vertexHandle, OpenMesh::Vec2f(
                        inputMesh->mTextureCoords[0][vertexIndex].x,
                        inputMesh->mTextureCoords[0][vertexIndex].y));

                    mesh.data(vertexHandle).tangent = OpenMesh::Vec3f(
                        inputMesh->mTangents[vertexIndex].x,
                        inputMesh->mTangents[vertexIndex].y,
                        inputMesh->mTangents[vertexIndex].z);

                    mesh.data(vertexHandle).biTangent = OpenMesh::Vec3f(
                        inputMesh->mBitangents[vertexIndex].x,
                        inputMesh->mBitangents[vertexIndex].y,
                        inputMesh->mBitangents[vertexIndex].z);

                    mesh.set_normal(vertexHandle, OpenMesh::Vec3f(
                        inputMesh->mNormals[vertexIndex].x,
                        inputMesh->mNormals[vertexIndex].y,
                        inputMesh->mNormals[vertexIndex].z));
                }

                auto meshVertexStart = mesh.n_vertices();
                std::vector<Mesh::VertexHandle> faceHandles(3);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        LockedWrite{ std::cerr } << String::Format("Non-triangular face encountered");
                        return false;
                    }

                    faceHandles.clear();
                    uint32_t edgeStartIndex = (faceIndex * 3);
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        faceHandles.push_back(mesh.vertex_handle(meshVertexStart + face.mIndices[edgeIndex]));
                    }

                    mesh.add_face(faceHandles);
                }
            }
        }
    }

    if (inputNode->mNumChildren > 0)
    {
        if (inputNode->mChildren == nullptr)
        {
            LockedWrite{ std::cerr } << String::Format("Invalid child list");
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            if (!getSceneMeshes(parameters, inputScene, inputNode->mChildren[childIndex], sceneMeshMap, boundingBox))
            {
                return false;
            }
        }
    }

    return true;
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{ std::cout } << String::Format("GEK Model Converter");

    FileSystem::Path fileNameInput;
    FileSystem::Path fileNameOutput;
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
            LockedWrite{ std::cerr } << String::Format("No arguments specified for command line parameter");
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
                LockedWrite{ std::cerr } << String::Format("Missing parameters for smoothAngle");
                return -__LINE__;
            }

			smoothingAngle = String::Convert(arguments[1], 80.0f);
        }
        else if (arguments[0] == "-unitsinfoot")
        {
            if (arguments.size() != 2)
            {
                LockedWrite{ std::cerr } << String::Format("Missing parameters for unitsInFoot");
                return -__LINE__;
            }

			parameters.feetPerUnit = (1.0f / String::Convert(arguments[1], 1.0f));
        }
        else if (arguments[0] == "-forcematerial" && ++argumentIndex < argumentCount)
        {
            parameters.forceMaterial = String::Narrow(argumentList[argumentIndex]);
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
        aiProcess_SortByPType |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        0;

    unsigned int textureProcessFlags =
        aiProcess_GenUVCoords |
        //aiProcess_GenUVCoords_Sphere |
        aiProcess_TransformUVCoords |
        0;

    unsigned int tangentProcessFlags =
        aiProcess_JoinIdenticalVertices |
        aiProcess_FindInvalidData |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_OptimizeGraph |
        0;

    aiPropertyStore *propertyStore = aiCreatePropertyStore();
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    aiSetImportPropertyFloat(propertyStore, AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothingAngle);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);
    aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
    auto inputScene = aiImportFileExWithProperties(fileNameInput.u8string().c_str(), importFlags, nullptr, propertyStore);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Unable to load inputScene with Assimp");
        return -__LINE__;
    }

    inputScene = aiApplyPostProcessing(inputScene, textureProcessFlags);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Unable to apply texture post processing with Assimp");
        return -__LINE__;
    }

    inputScene = aiApplyPostProcessing(inputScene, tangentProcessFlags);
    if (inputScene == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Unable to apply tangent post processing with Assimp");
        return -__LINE__;
    }

    if (!inputScene->HasMeshes())
    {
        LockedWrite{ std::cerr } << String::Format("Scene has no meshes");
        return -__LINE__;
    }

    if (!inputScene->HasMaterials())
    {
        LockedWrite{ std::cerr } << String::Format("Exporting to model requires materials in inputScene");
        return -__LINE__;
    }

    Shapes::AlignedBox boundingBox;
    std::unordered_map<std::string, Mesh> sceneMeshMap;
    if (!getSceneMeshes(parameters, inputScene, inputScene->mRootNode, sceneMeshMap, boundingBox))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(inputScene);

    auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
    auto dataPath(FileSystem::GetFileName(rootPath, "Data"));

	std::string texturesPath(String::GetLower(FileSystem::GetFileName(dataPath, "Textures").u8string()));
    auto materialsPath(FileSystem::GetFileName(dataPath, "Materials").u8string());

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
            JSON::Instance materialNode = JSON::Load(filePath);
            auto shaderNode = materialNode.get("shader");
            auto dataNode = shaderNode.get("data");
            auto albedoNode = dataNode.get("albedo");
            std::string albedoPath(albedoNode.get("file").convert(String::Empty));
            std::string materialName(String::GetLower(filePath.withoutExtension().u8string().substr(materialsPath.size() + 1)));
            albedoToMaterialMap[albedoPath] = materialName;
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
    if (albedoToMaterialMap.empty())
    {
        LockedWrite{ std::cerr } << String::Format("Unable to locate any materials");
        return -__LINE__;
    }

    std::unordered_map<std::string, Mesh> materialMeshMap;
    for (auto const &modelAlbedo : sceneMeshMap)
    {
		std::string albedoName(String::GetLower(FileSystem::Path(modelAlbedo.first).withoutExtension().u8string()));
        LockedWrite{ std::cout } << String::Format("Found Albedo: %v", albedoName);
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

        auto materialAlebedoSearch = albedoToMaterialMap.find(albedoName);
        if (materialAlebedoSearch == std::end(albedoToMaterialMap))
        {
            LockedWrite{ std::cerr } << String::Format("! Unable to find material for albedo: %v", albedoName.c_str());
        }
        else
        {
            materialMeshMap[materialAlebedoSearch->second] = modelAlbedo.second;
        }
    }

	LockedWrite{ std::cout } << String::Format("> Num. Parts: %v", materialMeshMap.size());
    LockedWrite{ std::cout } << String::Format("< Size: Minimum[%v, %v, %v]", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
    LockedWrite{ std::cout } << String::Format("< Size: Maximum[%v, %v, %v]", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput.c_str(), L"wb");
    if (file == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Unable to create output file");
        return -__LINE__;
    }

    Header header;
    header.meshCount = materialMeshMap.size();
    header.boundingBox = boundingBox;
    fwrite(&header, sizeof(Header), 1, file);
    for (auto const &materialMesh : materialMeshMap)
    {
		std::string material = materialMesh.first;
        auto &mesh = materialMesh.second;

		LockedWrite{ std::cout } << String::Format("-    Material: %v", material);
        LockedWrite{ std::cout } << String::Format("       Num. Vertices: %v", mesh.n_vertices());
        LockedWrite{ std::cout } << String::Format("       Num. Faces: %v", mesh.n_faces());

        Header::Material materialHeader;
        std::strncpy(materialHeader.name, material.c_str(), 63);
        materialHeader.vertexCount = mesh.n_vertices();
        materialHeader.indexCount = (mesh.n_faces() * 3);
        fwrite(&materialHeader, sizeof(Header::Material), 1, file);
    }

    for (auto const &materialMesh : materialMeshMap)
    {
        auto &mesh = materialMesh.second;

        fwrite(&mesh.faces().begin(), sizeof(uint16_t), (mesh.n_faces() * 3), file);
        fwrite(mesh.points(), sizeof(Math::Float3), mesh.n_vertices(), file);
        fwrite(mesh.texcoords2D(), sizeof(Math::Float2), mesh.n_vertices(), file);
        //fwrite(material.second.vertexTangentList.data(), sizeof(Math::Float3), material.second.vertexTangentList.size(), file);
        //fwrite(material.second.vertexBiTangentList.data(), sizeof(Math::Float3), material.second.vertexBiTangentList.size(), file);
        fwrite(mesh.vertex_normals(), sizeof(Math::Float3), mesh.n_vertices(), file);
    }

    fclose(file);

    return 0;
}
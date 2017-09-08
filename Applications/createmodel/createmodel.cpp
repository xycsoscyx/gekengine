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
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>
#include <OpenMesh/Tools/Decimater/ModHausdorffT.hh>
#include <OpenMesh/Tools/Decimater/ModAspectRatioT.hh>
#include <OpenMesh/Tools/Decimater/ModNormalDeviationT.hh>
#include <OpenMesh/Tools/Decimater/ModNormalFlippingT.hh>

using namespace Gek;

struct Header
{
    struct Material
    {
        char name[64] = "";
        struct Level
        {
            uint32_t vertexCount = 0;
            uint32_t indexCount = 0;
        };
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 0;
    uint16_t version = 7;
    uint8_t levelCount = 5;

    Shapes::AlignedBox boundingBox;

    uint32_t meshCount;
};

struct Traits : OpenMesh::DefaultTraits
{
    typedef Math::Float3 Tangent;
    typedef Math::Float3 BiTangent;

    VertexAttributes(OpenMesh::Attributes::Status | OpenMesh::Attributes::Normal | OpenMesh::Attributes::TexCoord2D);
    HalfedgeAttributes(OpenMesh::Attributes::Status | OpenMesh::Attributes::PrevHalfedge | OpenMesh::Attributes::TexCoord2D);
    EdgeAttributes(OpenMesh::Attributes::Status);
    FaceAttributes(OpenMesh::Attributes::Status | OpenMesh::Attributes::Normal);

    VertexTraits
    {
        Tangent tangent;
        BiTangent biTangent;
    };
};

struct Mesh
    : public OpenMesh::TriMesh_ArrayKernelT<Traits>
{
    std::string diffuse;
    std::string material;
};

using Decimater = OpenMesh::Decimater::DecimaterT<Mesh>;
using QuadraticModule = OpenMesh::Decimater::ModQuadricT<Mesh>::Handle;
using HausdorffModule = OpenMesh::Decimater::ModHausdorffT<Mesh>::Handle;
using AspectModule = OpenMesh::Decimater::ModAspectRatioT<Mesh>::Handle;
using DeviationModule = OpenMesh::Decimater::ModNormalDeviationT<Mesh>::Handle;
using FlippingModule = OpenMesh::Decimater::ModNormalFlippingT<Mesh>::Handle;

struct Parameters
{
    float feetPerUnit = 1.0f;
};

bool GetNodeMeshes(const Parameters &parameters, const aiScene *inputScene, const aiNode *inputNode, std::vector<std::shared_ptr<Mesh>> &meshList, Shapes::AlignedBox &boundingBox)
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

                auto mesh = std::make_shared<Mesh>();
                meshList.push_back(mesh);

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                mesh->diffuse = sceneDiffuseMaterial.C_Str();

                std::vector<OpenMesh::VertexHandle> vertexHandleList;
                vertexHandleList.reserve(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertexHandle = mesh->add_vertex(OpenMesh::Vec3f(
                        (inputMesh->mVertices[vertexIndex].x * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].y * parameters.feetPerUnit),
                        (inputMesh->mVertices[vertexIndex].z * parameters.feetPerUnit)));
                    boundingBox.extend(Math::Float3(mesh->point(vertexHandle).data()));
                    vertexHandleList.push_back(vertexHandle);

                    mesh->set_texcoord2D(vertexHandle, OpenMesh::Vec2f(
                        inputMesh->mTextureCoords[0][vertexIndex].x,
                        inputMesh->mTextureCoords[0][vertexIndex].y));

                    mesh->data(vertexHandle).tangent = Math::Float3(
                        inputMesh->mTangents[vertexIndex].x,
                        inputMesh->mTangents[vertexIndex].y,
                        inputMesh->mTangents[vertexIndex].z);

                    mesh->data(vertexHandle).biTangent = Math::Float3(
                        inputMesh->mBitangents[vertexIndex].x,
                        inputMesh->mBitangents[vertexIndex].y,
                        inputMesh->mBitangents[vertexIndex].z);

                    mesh->set_normal(vertexHandle, OpenMesh::Vec3f(
                        inputMesh->mNormals[vertexIndex].x,
                        inputMesh->mNormals[vertexIndex].y,
                        inputMesh->mNormals[vertexIndex].z));
                }

                std::vector<Mesh::VertexHandle> faceHandles(3);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        LockedWrite{ std::cerr } << String::Format("Non-triangular face encountered");
                        return false;
                    }

                    uint32_t edgeStartIndex = (faceIndex * 3);
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        faceHandles[edgeIndex] = vertexHandleList[face.mIndices[edgeIndex]];
                    }

                    mesh->add_face(faceHandles);
                }

                meshList.push_back(mesh);
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
            if (!GetNodeMeshes(parameters, inputScene, inputNode->mChildren[childIndex], meshList, boundingBox))
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
    }

    aiLogStream logStream;
    logStream.callback = [](char const *message, char *user) -> void
    {
        std::string trimmedMessage(message);
        trimmedMessage = trimmedMessage.substr(0, trimmedMessage.size() - 1);
        LockedWrite{ std::cerr } << String::Format("Assimp: %v", trimmedMessage);
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
        //aiProcess_OptimizeMeshes |
        aiProcess_RemoveComponent |
        aiProcess_SplitLargeMeshes |
        aiProcess_PreTransformVertices |
        aiProcess_Triangulate |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        0;

    unsigned int textureProcessFlags =
        aiProcess_GenUVCoords |
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
    std::vector<std::shared_ptr<Mesh>> meshList;
    if (!GetNodeMeshes(parameters, inputScene, inputScene->mRootNode, meshList, boundingBox))
    {
        return -__LINE__;
    }

    aiReleasePropertyStore(propertyStore);
    aiReleaseImport(inputScene);

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
        LockedWrite{ std::cerr } << String::Format("Unable to locate any materials");
        return -__LINE__;
    }

	LockedWrite{ std::cout } << String::Format("> Num. Meshes: %v", meshList.size());
    LockedWrite{ std::cout } << String::Format("< Size: Minimum[%v, %v, %v]", boundingBox.minimum.x, boundingBox.minimum.y, boundingBox.minimum.z);
    LockedWrite{ std::cout } << String::Format("< Size: Maximum[%v, %v, %v]", boundingBox.maximum.x, boundingBox.maximum.y, boundingBox.maximum.z);

    FILE *file = nullptr;
    _wfopen_s(&file, fileNameOutput.c_str(), L"wb");
    if (file == nullptr)
    {
        LockedWrite{ std::cerr } << String::Format("Unable to create output file");
        return -__LINE__;
    }

    std::vector<std::shared_ptr<Mesh>> finalMeshList;
    for (auto &mesh : meshList)
    {
        FileSystem::Path diffusePath(mesh->diffuse);
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
            continue;
        }

        mesh->material =  materialAlebedoSearch->second;
        finalMeshList.push_back(mesh);
    }

    Header header;
    header.meshCount = finalMeshList.size();
    header.boundingBox = boundingBox;
    fwrite(&header, sizeof(Header), 1, file);

    for (auto &mesh : finalMeshList)
    {
        Header::Material materialHeader;
        std::strncpy(materialHeader.name, mesh->material.c_str(), 63);
        fwrite(&materialHeader, sizeof(Header::Material), 1, file);
    }

    for (auto &mesh : finalMeshList)
    {
        Decimater decimater(*mesh);

        QuadraticModule quadraticModule;
        decimater.add(quadraticModule);
        decimater.module(quadraticModule).set_binary(true);
        decimater.module(quadraticModule).set_max_err(0.001);

        HausdorffModule hausdorffModule;
        //decimater.add(hausdorffModule);
        //decimater.module(hausdorffModule).set_binary(true);
        //decimater.module(hausdorffModule).set_tolerance(0.1f);

        FlippingModule flippingModule;
        decimater.add(flippingModule);

        AspectModule aspectModule;
        //decimater.add(aspectModule);

        if (!decimater.initialize())
        {
            LockedWrite{ std::cerr } << String::Format("! Failed to initiaize decimator: %v", mesh->material);
            return -__LINE__;
        }

        LockedWrite{ std::cout } << String::Format("-    Material: %v", mesh->material);

        uint32_t originalFaceCount = mesh->n_faces();
        uint32_t decimateStepCount = (originalFaceCount / (header.levelCount + 1));
        uint32_t decimateFaceCount = originalFaceCount;
        for (uint32_t level = 0; level < header.levelCount; ++level, decimateFaceCount -= decimateStepCount)
        {
            if (level > 0)
            {
                decimater.decimate_to_faces(0, decimateFaceCount);
                mesh->garbage_collection();
            }

            LockedWrite{ std::cout } << String::Format("       Level: %v", level);
            LockedWrite{ std::cout } << String::Format("           Num. Vertices: %v", mesh->n_vertices());
            LockedWrite{ std::cout } << String::Format("           Num. Faces: %v", mesh->n_faces());

            //mesh->update_normals();
            std::vector<uint16_t> indexList;
            indexList.reserve(mesh->n_faces() * 3);
            for (auto faceIterator = mesh->faces_sbegin(); faceIterator != mesh->faces_end(); ++faceIterator)
            {
                std::vector<uint16_t> faceIndexList;
                for (auto vertexIterator = mesh->fv_iter(*faceIterator); vertexIterator->is_valid(); ++vertexIterator)
                {
                    faceIndexList.push_back(vertexIterator->idx());
                }

                if (faceIndexList.size() == 3)
                {
                    indexList.insert(std::end(indexList), std::begin(faceIndexList), std::end(faceIndexList));
                }
                else
                {
                    LockedWrite{ std::cout } << String::Format("! Non-triangular face encountered");
                }
            }

            std::vector<Math::Float3> pointList;
            std::vector<Math::Float2> texCoordList;
            std::vector<Math::Float3> tangentList;
            std::vector<Math::Float3> biTangentList;
            std::vector<Math::Float3> normalList;
            pointList.reserve(mesh->n_vertices());
            texCoordList.reserve(mesh->n_vertices());
            tangentList.reserve(mesh->n_vertices());
            biTangentList.reserve(mesh->n_vertices());
            normalList.reserve(mesh->n_vertices());
            for (auto vertexIterator = mesh->vertices_sbegin(); vertexIterator != mesh->vertices_end(); ++vertexIterator)
            {
                pointList.push_back(Math::Float3(mesh->point(*vertexIterator).data()));
                texCoordList.push_back(Math::Float2(mesh->texcoord2D(*vertexIterator).data()));
                auto &meshData = mesh->data(*vertexIterator);
                tangentList.push_back(meshData.tangent);
                biTangentList.push_back(meshData.biTangent);
                normalList.push_back(Math::Float3(mesh->normal(*vertexIterator).data()));
            }

            Header::Material::Level levelHeader;
            levelHeader.vertexCount = pointList.size();
            levelHeader.indexCount = indexList.size();
            fwrite(&levelHeader, sizeof(Header::Material::Level), 1, file);
            fwrite(indexList.data(), sizeof(uint16_t), indexList.size(), file);
            fwrite(pointList.data(), sizeof(Math::Float3), pointList.size(), file);
            fwrite(texCoordList.data(), sizeof(Math::Float2), texCoordList.size(), file);
            fwrite(tangentList.data(), sizeof(Math::Float3), tangentList.size(), file);
            fwrite(biTangentList.data(), sizeof(Math::Float3), biTangentList.size(), file);
            fwrite(normalList.data(), sizeof(Math::Float3), normalList.size(), file);
        }
    }

    fclose(file);

    return 0;
}
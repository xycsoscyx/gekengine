#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <argparse/argparse.hpp>
#include <assimp/cimport.h>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Gek;

struct Header
{
    struct Material
    {
        char name[64] = "";
    };

    struct Mesh
    {
        uint32_t materialIndex;
        uint32_t faceCount;
        uint32_t pointCount;
    };

    uint32_t identifier = *(uint32_t *)"GEKX";
    uint16_t type = 2;
    uint16_t version = 3;

    uint32_t materialCount = 0;
    uint32_t meshCount = 0;
};

struct Mesh
{
    struct Face
    {
        int32_t data[3];
        int32_t &operator[](size_t index)
        {
            return data[index];
        }

        const int32_t &operator[](size_t index) const
        {
            return data[index];
        }
    };

    std::string material;
    std::vector<Math::Float3> pointList;
    std::vector<Face> faceList;
};

struct Model
{
    Shapes::AlignedBox boundingBox;
    std::vector<Mesh> meshList;
};

struct Parameters
{
    std::string sourceName;
    std::string targetName;
    float feetPerUnit;
};

bool GetModels(Context *context, Parameters const &parameters, aiScene const *inputScene, aiNode const *inputNode, aiMatrix4x4 const &parentTransform, Model &model, std::function<std::string(const std::string &, const std::string &)> findMaterialForMesh)
{
    if (inputNode == nullptr)
    {
        context->log(Context::Error, "Invalid scene node");
        return false;
    }

    aiMatrix4x4 transform(parentTransform * inputNode->mTransformation);
    if (inputNode->mNumMeshes > 0)
    {
        if (inputNode->mMeshes == nullptr)
        {
            context->log(Context::Error, "Invalid mesh list");
            return false;
        }

        std::string name = inputNode->mName.C_Str();
        context->log(Context::Info, "Found Assimp Model: {}", name);
        for (uint32_t meshIndex = 0; meshIndex < inputNode->mNumMeshes; ++meshIndex)
        {
            uint32_t nodeMeshIndex = inputNode->mMeshes[meshIndex];
            if (nodeMeshIndex >= inputScene->mNumMeshes)
            {
                context->log(Context::Error, "Invalid mesh index");
                continue;
            }

            const aiMesh *inputMesh = inputScene->mMeshes[nodeMeshIndex];
            if (inputMesh->mNumFaces > 0)
            {
                if (inputMesh->mFaces == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh face list");
                    continue;
                }

                if (inputMesh->mVertices == nullptr)
                {
                    context->log(Context::Error, "Invalid inputMesh vertex list");
                    continue;
                }

                Mesh mesh;

                aiString sceneDiffuseMaterial;
                const aiMaterial *sceneMaterial = inputScene->mMaterials[inputMesh->mMaterialIndex];
                sceneMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &sceneDiffuseMaterial);
                std::string diffuseName = sceneDiffuseMaterial.C_Str();
                mesh.material = findMaterialForMesh(parameters.sourceName, diffuseName);
                if (mesh.material.empty())
                {
                    continue;
                }

                mesh.pointList.resize(inputMesh->mNumVertices);
                for (uint32_t vertexIndex = 0; vertexIndex < inputMesh->mNumVertices; ++vertexIndex)
                {
                    auto vertex = inputMesh->mVertices[vertexIndex];
                    aiTransformVecByMatrix4(&vertex, &transform);
                    mesh.pointList[vertexIndex].set(vertex.x, vertex.y, vertex.z);
                    mesh.pointList[vertexIndex] *= parameters.feetPerUnit;
                    model.boundingBox.extend(mesh.pointList[vertexIndex]);
                }

                mesh.faceList.reserve(inputMesh->mNumFaces);
                for (uint32_t faceIndex = 0; faceIndex < inputMesh->mNumFaces; ++faceIndex)
                {
                    const aiFace &face = inputMesh->mFaces[faceIndex];
                    if (face.mNumIndices != 3)
                    {
                        context->log(Context::Error, "Skipping non-triangular face, face: {}, {} indices", faceIndex, face.mNumIndices);
                        continue;
                    }

                    Mesh::Face meshFace;
                    for (uint32_t edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
                    {
                        meshFace[edgeIndex] = face.mIndices[edgeIndex];
                    }

                    mesh.faceList.push_back(meshFace);
                }

                model.meshList.push_back(mesh);
            }
        }
    }

    if (inputNode->mNumChildren > 0)
    {
        if (inputNode->mChildren == nullptr)
        {
            context->log(Context::Error, "Invalid child list");
            return false;
        }

        for (uint32_t childIndex = 0; childIndex < inputNode->mNumChildren; ++childIndex)
        {
            if (!GetModels(context, parameters, inputScene, inputNode->mChildren[childIndex], transform, model, findMaterialForMesh))
            {
                return false;
            }
        }
    }

    return true;
}

bool SanitizeTreeModel(Context *context, Model &model, std::string const &modelName)
{
    struct QuantizedPoint
    {
        long long x;
        long long y;
        long long z;
    };

    struct FaceLocation
    {
        size_t meshIndex;
        size_t faceIndex;
    };

    auto isFiniteFloat3 = [](Math::Float3 const &p) -> bool
    {
        return std::isfinite(static_cast<double>(p.x)) && std::isfinite(static_cast<double>(p.y)) && std::isfinite(static_cast<double>(p.z));
    };

    auto triangleAreaSquared = [](Math::Float3 const &p0, Math::Float3 const &p1, Math::Float3 const &p2) -> double
    {
        const double ax = static_cast<double>(p1.x) - static_cast<double>(p0.x);
        const double ay = static_cast<double>(p1.y) - static_cast<double>(p0.y);
        const double az = static_cast<double>(p1.z) - static_cast<double>(p0.z);

        const double bx = static_cast<double>(p2.x) - static_cast<double>(p0.x);
        const double by = static_cast<double>(p2.y) - static_cast<double>(p0.y);
        const double bz = static_cast<double>(p2.z) - static_cast<double>(p0.z);

        const double cx = ay * bz - az * by;
        const double cy = az * bx - ax * bz;
        const double cz = ax * by - ay * bx;
        return (cx * cx) + (cy * cy) + (cz * cz);
    };

    auto quantizePoint = [](Math::Float3 const &p, double scale) -> QuantizedPoint
    {
        QuantizedPoint q;
        q.x = static_cast<long long>(std::llround(static_cast<double>(p.x) * scale));
        q.y = static_cast<long long>(std::llround(static_cast<double>(p.y) * scale));
        q.z = static_cast<long long>(std::llround(static_cast<double>(p.z) * scale));
        return q;
    };

    auto sameQuantizedPoint = [](QuantizedPoint const &a, QuantizedPoint const &b) -> bool
    {
        return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
    };

    auto lessQuantizedPoint = [](QuantizedPoint const &a, QuantizedPoint const &b) -> bool
    {
        if (a.x != b.x)
        {
            return a.x < b.x;
        }
        if (a.y != b.y)
        {
            return a.y < b.y;
        }
        return a.z < b.z;
    };

    auto quantizedTriangleAreaSquared = [](QuantizedPoint const &p0, QuantizedPoint const &p1, QuantizedPoint const &p2, double weldTolerance) -> double
    {
        const double ax = static_cast<double>(p1.x - p0.x) * weldTolerance;
        const double ay = static_cast<double>(p1.y - p0.y) * weldTolerance;
        const double az = static_cast<double>(p1.z - p0.z) * weldTolerance;

        const double bx = static_cast<double>(p2.x - p0.x) * weldTolerance;
        const double by = static_cast<double>(p2.y - p0.y) * weldTolerance;
        const double bz = static_cast<double>(p2.z - p0.z) * weldTolerance;

        const double cx = ay * bz - az * by;
        const double cy = az * bx - ax * bz;
        const double cz = ax * by - ay * bx;
        return (cx * cx) + (cy * cy) + (cz * cz);
    };

    constexpr double MinTriangleAreaSquared = 1.0e-12;
    constexpr double PolygonSoupWeldTolerance = 1.0e-6;
    constexpr double PolygonSoupWeldScale = 1.0 / PolygonSoupWeldTolerance;
    constexpr double MinWeldedEdgeSquared = PolygonSoupWeldTolerance * PolygonSoupWeldTolerance;
    constexpr double MinQuantizedTriangleAreaSquared = MinWeldedEdgeSquared * MinWeldedEdgeSquared;
    constexpr double MinTriangleEdgeSquared = 1.0e-10;
    constexpr double MinTriangleQuality = 1.0e-6;
    constexpr double MaxEdgeNormalDot = 2.0e-1;

    size_t invalidFaceCount = 0;
    size_t duplicateIndexFaceCount = 0;
    size_t nonFiniteFaceCount = 0;
    size_t weldedEdgeFaceCount = 0;
    size_t weldedDegenerateFaceCount = 0;
    size_t duplicateWeldedTriangleFaceCount = 0;
    size_t nonManifoldFaceCount = 0;
    size_t invalidAdjacencyFaceCount = 0;
    size_t shortEdgeFaceCount = 0;
    size_t sliverFaceCount = 0;
    size_t degenerateFaceCount = 0;

    std::map<std::array<long long, 9>, size_t> triangleFaceMap;
    std::map<std::array<long long, 6>, std::vector<size_t>> edgeFaceMap;
    std::vector<std::array<double, 3>> triangleNormals;
    std::vector<FaceLocation> acceptedFaces;

    std::vector<std::vector<bool>> faceKeepMask(model.meshList.size());
    for (size_t meshIndex = 0; meshIndex < model.meshList.size(); ++meshIndex)
    {
        faceKeepMask[meshIndex].assign(model.meshList[meshIndex].faceList.size(), false);
    }

    for (size_t meshIndex = 0; meshIndex < model.meshList.size(); ++meshIndex)
    {
        auto &mesh = model.meshList[meshIndex];
        for (size_t faceIndex = 0; faceIndex < mesh.faceList.size(); ++faceIndex)
        {
            auto const &face = mesh.faceList[faceIndex];
            int32_t i0 = face[0];
            int32_t i1 = face[1];
            int32_t i2 = face[2];

            if (i0 < 0 || i1 < 0 || i2 < 0 ||
                static_cast<size_t>(i0) >= mesh.pointList.size() ||
                static_cast<size_t>(i1) >= mesh.pointList.size() ||
                static_cast<size_t>(i2) >= mesh.pointList.size())
            {
                ++invalidFaceCount;
                continue;
            }

            if ((i0 == i1) || (i1 == i2) || (i2 == i0))
            {
                ++duplicateIndexFaceCount;
                continue;
            }

            Math::Float3 const &p0 = mesh.pointList[i0];
            Math::Float3 const &p1 = mesh.pointList[i1];
            Math::Float3 const &p2 = mesh.pointList[i2];

            if (!isFiniteFloat3(p0) || !isFiniteFloat3(p1) || !isFiniteFloat3(p2))
            {
                ++nonFiniteFaceCount;
                continue;
            }

            QuantizedPoint const qp0 = quantizePoint(p0, PolygonSoupWeldScale);
            QuantizedPoint const qp1 = quantizePoint(p1, PolygonSoupWeldScale);
            QuantizedPoint const qp2 = quantizePoint(p2, PolygonSoupWeldScale);
            if (sameQuantizedPoint(qp0, qp1) || sameQuantizedPoint(qp1, qp2) || sameQuantizedPoint(qp2, qp0))
            {
                ++weldedEdgeFaceCount;
                continue;
            }

            if (quantizedTriangleAreaSquared(qp0, qp1, qp2, PolygonSoupWeldTolerance) <= MinQuantizedTriangleAreaSquared)
            {
                ++weldedDegenerateFaceCount;
                continue;
            }

            QuantizedPoint keyP0 = qp0;
            QuantizedPoint keyP1 = qp1;
            QuantizedPoint keyP2 = qp2;
            if (lessQuantizedPoint(keyP1, keyP0))
            {
                std::swap(keyP0, keyP1);
            }
            if (lessQuantizedPoint(keyP2, keyP1))
            {
                std::swap(keyP1, keyP2);
            }
            if (lessQuantizedPoint(keyP1, keyP0))
            {
                std::swap(keyP0, keyP1);
            }

            std::array<long long, 9> const triKey = {
                keyP0.x, keyP0.y, keyP0.z,
                keyP1.x, keyP1.y, keyP1.z,
                keyP2.x, keyP2.y, keyP2.z
            };
            if (triangleFaceMap.find(triKey) != triangleFaceMap.end())
            {
                ++duplicateWeldedTriangleFaceCount;
                continue;
            }

            const double e01x = static_cast<double>(p1.x) - static_cast<double>(p0.x);
            const double e01y = static_cast<double>(p1.y) - static_cast<double>(p0.y);
            const double e01z = static_cast<double>(p1.z) - static_cast<double>(p0.z);
            const double e12x = static_cast<double>(p2.x) - static_cast<double>(p1.x);
            const double e12y = static_cast<double>(p2.y) - static_cast<double>(p1.y);
            const double e12z = static_cast<double>(p2.z) - static_cast<double>(p1.z);
            const double e20x = static_cast<double>(p0.x) - static_cast<double>(p2.x);
            const double e20y = static_cast<double>(p0.y) - static_cast<double>(p2.y);
            const double e20z = static_cast<double>(p0.z) - static_cast<double>(p2.z);

            const double edge01Squared = (e01x * e01x) + (e01y * e01y) + (e01z * e01z);
            const double edge12Squared = (e12x * e12x) + (e12y * e12y) + (e12z * e12z);
            const double edge20Squared = (e20x * e20x) + (e20y * e20y) + (e20z * e20z);
            if (edge01Squared <= MinWeldedEdgeSquared ||
                edge12Squared <= MinWeldedEdgeSquared ||
                edge20Squared <= MinWeldedEdgeSquared)
            {
                ++weldedEdgeFaceCount;
                continue;
            }

            if (edge01Squared <= MinTriangleEdgeSquared ||
                edge12Squared <= MinTriangleEdgeSquared ||
                edge20Squared <= MinTriangleEdgeSquared)
            {
                ++shortEdgeFaceCount;
                continue;
            }

            const double areaSquared = triangleAreaSquared(p0, p1, p2);
            if (areaSquared <= MinTriangleAreaSquared)
            {
                ++degenerateFaceCount;
                continue;
            }

            const double maxEdgeSquared = std::max(edge01Squared, std::max(edge12Squared, edge20Squared));
            if (maxEdgeSquared > MinTriangleEdgeSquared)
            {
                const double quality = areaSquared / (maxEdgeSquared * maxEdgeSquared);
                if (quality <= MinTriangleQuality)
                {
                    ++sliverFaceCount;
                    continue;
                }
            }

            const double e02x = static_cast<double>(p2.x) - static_cast<double>(p0.x);
            const double e02y = static_cast<double>(p2.y) - static_cast<double>(p0.y);
            const double e02z = static_cast<double>(p2.z) - static_cast<double>(p0.z);
            const double nx = (e01y * e02z) - (e01z * e02y);
            const double ny = (e01z * e02x) - (e01x * e02z);
            const double nz = (e01x * e02y) - (e01y * e02x);
            const double normalLength = std::sqrt((nx * nx) + (ny * ny) + (nz * nz));
            if (!std::isfinite(normalLength) || normalLength <= 0.0)
            {
                ++degenerateFaceCount;
                continue;
            }

            size_t const triIndex = acceptedFaces.size();
            acceptedFaces.push_back({ meshIndex, faceIndex });
            faceKeepMask[meshIndex][faceIndex] = true;
            triangleNormals.push_back({ nx / normalLength, ny / normalLength, nz / normalLength });
            triangleFaceMap[triKey] = triIndex;

            auto registerEdge = [&](QuantizedPoint a, QuantizedPoint b)
            {
                if (lessQuantizedPoint(b, a))
                {
                    std::swap(a, b);
                }
                std::array<long long, 6> const edgeKey = {
                    a.x, a.y, a.z,
                    b.x, b.y, b.z
                };
                edgeFaceMap[edgeKey].push_back(triIndex);
            };

            registerEdge(qp0, qp1);
            registerEdge(qp1, qp2);
            registerEdge(qp2, qp0);
        }
    }

    std::vector<bool> rejected(acceptedFaces.size(), false);
    for (auto const &edgeEntry : edgeFaceMap)
    {
        auto const &facesOnEdge = edgeEntry.second;
        if (facesOnEdge.size() > 2)
        {
            for (size_t const faceIndex : facesOnEdge)
            {
                if (!rejected[faceIndex])
                {
                    rejected[faceIndex] = true;
                    ++nonManifoldFaceCount;
                }
            }
        }
        else if (facesOnEdge.size() == 2)
        {
            size_t const face0 = facesOnEdge[0];
            size_t const face1 = facesOnEdge[1];
            if ((face0 < triangleNormals.size()) && (face1 < triangleNormals.size()))
            {
                std::array<long long, 6> const &edgeKey = edgeEntry.first;
                double edgeX = static_cast<double>(edgeKey[3] - edgeKey[0]) * PolygonSoupWeldTolerance;
                double edgeY = static_cast<double>(edgeKey[4] - edgeKey[1]) * PolygonSoupWeldTolerance;
                double edgeZ = static_cast<double>(edgeKey[5] - edgeKey[2]) * PolygonSoupWeldTolerance;
                double edgeLengthSquared = (edgeX * edgeX) + (edgeY * edgeY) + (edgeZ * edgeZ);
                if (edgeLengthSquared <= MinWeldedEdgeSquared)
                {
                    if (!rejected[face0])
                    {
                        rejected[face0] = true;
                        ++invalidAdjacencyFaceCount;
                    }
                    if (!rejected[face1])
                    {
                        rejected[face1] = true;
                        ++invalidAdjacencyFaceCount;
                    }
                    continue;
                }

                const double inverseEdgeLength = 1.0 / std::sqrt(edgeLengthSquared);
                edgeX *= inverseEdgeLength;
                edgeY *= inverseEdgeLength;
                edgeZ *= inverseEdgeLength;

                auto const &normal0 = triangleNormals[face0];
                auto const &normal1 = triangleNormals[face1];
                const double edgeDotNormal0 = std::fabs((edgeX * normal0[0]) + (edgeY * normal0[1]) + (edgeZ * normal0[2]));
                const double edgeDotNormal1 = std::fabs((edgeX * normal1[0]) + (edgeY * normal1[1]) + (edgeZ * normal1[2]));

                if (!std::isfinite(edgeDotNormal0) || !std::isfinite(edgeDotNormal1) ||
                    (edgeDotNormal0 >= MaxEdgeNormalDot) || (edgeDotNormal1 >= MaxEdgeNormalDot))
                {
                    if (!rejected[face0])
                    {
                        rejected[face0] = true;
                        ++invalidAdjacencyFaceCount;
                    }
                    if (!rejected[face1])
                    {
                        rejected[face1] = true;
                        ++invalidAdjacencyFaceCount;
                    }
                }
            }
        }
    }

    for (size_t triIndex = 0; triIndex < acceptedFaces.size(); ++triIndex)
    {
        if (rejected[triIndex])
        {
            FaceLocation const &location = acceptedFaces[triIndex];
            faceKeepMask[location.meshIndex][location.faceIndex] = false;
        }
    }

    for (size_t meshIndex = 0; meshIndex < model.meshList.size(); ++meshIndex)
    {
        auto &mesh = model.meshList[meshIndex];
        auto const &keepMask = faceKeepMask[meshIndex];

        std::vector<Mesh::Face> sanitizedFaces;
        sanitizedFaces.reserve(mesh.faceList.size());
        for (size_t faceIndex = 0; faceIndex < mesh.faceList.size(); ++faceIndex)
        {
            if (keepMask[faceIndex])
            {
                sanitizedFaces.push_back(mesh.faceList[faceIndex]);
            }
        }

        mesh.faceList.swap(sanitizedFaces);
    }

    if (invalidFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} invalid faces while creating tree physics model: {}", invalidFaceCount, modelName);
    }
    if (duplicateIndexFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} duplicate-index faces while creating tree physics model: {}", duplicateIndexFaceCount, modelName);
    }
    if (nonFiniteFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} non-finite faces while creating tree physics model: {}", nonFiniteFaceCount, modelName);
    }
    if (weldedEdgeFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} pre-weld-collapsed faces while creating tree physics model: {}", weldedEdgeFaceCount, modelName);
    }
    if (weldedDegenerateFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} weld-space degenerate faces while creating tree physics model: {}", weldedDegenerateFaceCount, modelName);
    }
    if (duplicateWeldedTriangleFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} duplicate weld-space triangles while creating tree physics model: {}", duplicateWeldedTriangleFaceCount, modelName);
    }
    if (nonManifoldFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} non-manifold welded faces while creating tree physics model: {}", nonManifoldFaceCount, modelName);
    }
    if (invalidAdjacencyFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} invalid-adjacency faces while creating tree physics model: {}", invalidAdjacencyFaceCount, modelName);
    }
    if (shortEdgeFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} short-edge faces while creating tree physics model: {}", shortEdgeFaceCount, modelName);
    }
    if (sliverFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} sliver faces while creating tree physics model: {}", sliverFaceCount, modelName);
    }
    if (degenerateFaceCount > 0)
    {
        context->log(Context::Warning, "Skipped {} degenerate faces while creating tree physics model: {}", degenerateFaceCount, modelName);
    }

    if ((nonManifoldFaceCount > 0) || (duplicateWeldedTriangleFaceCount > 0) || (invalidAdjacencyFaceCount > 0))
    {
        context->log(Context::Error, "Rejected tree physics model due to invalid welded topology or adjacency: {}", modelName);
        return false;
    }

    size_t totalFaceCount = 0;
    std::vector<Mesh> sanitizedMeshList;
    sanitizedMeshList.reserve(model.meshList.size());
    for (auto &mesh : model.meshList)
    {
        if (!mesh.faceList.empty())
        {
            totalFaceCount += mesh.faceList.size();
            sanitizedMeshList.push_back(std::move(mesh));
        }
    }
    model.meshList.swap(sanitizedMeshList);

    if (totalFaceCount == 0)
    {
        context->log(Context::Error, "No valid faces in tree physics model: {}", modelName);
        return false;
    }

    context->log(Context::Info, "Sanitized tree physics model {} to {} mesh(es), {} face(s)", modelName, model.meshList.size(), totalFaceCount);
    return true;
}

int main(int argumentCount, char const *const argumentList[], char const *const environmentVariableList)
{
    ContextPtr context(Context::Create(nullptr));

    argparse::ArgumentParser program("GEK Tree Converter", "1.0");

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
            arguments.push_back(argumentList[argumentIndex]);
        }

        program.parse_args(arguments);
    }
    catch (const std::runtime_error &err)
    {
        if (context)
        {
            std::ostringstream usageStream;
            usageStream << program;
            context->log(Context::Error, "{}", err.what());
            context->log(Context::Error, "{}", usageStream.str());
        }
        return 1;
    }

    Parameters parameters;
    parameters.sourceName = program.get<std::string>("--input");
    parameters.targetName = program.get<std::string>("--output");
    parameters.feetPerUnit = (1.0f / program.get<float>("--unitsperfoot"));

    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto cachePath(FileSystem::GetCacheFromModule());
    auto rootPath(cachePath.getParentPath());
    cachePath.setWorkingDirectory();

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    if (context)
    {
        context->log(Context::Info, "GEK Tree Converter");
        context->setCachePath(cachePath);

        auto gekDataPath = std::getenv("gek_data_path");
        if (gekDataPath)
        {
            context->addDataPath(gekDataPath);
        }

        context->addDataPath(rootPath / "data");
        context->addDataPath(rootPath.getString());

        aiLogStream logStream;
        logStream.callback = [](char const *message, char *user) -> void
        {
            Context *context = reinterpret_cast<Context *>(user);
            context->log(Context::Info, message);
        };

        logStream.user = reinterpret_cast<char *>(context.get());
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
            aiProcess_FindDegenerates |
            aiProcess_ValidateDataStructure |
            aiProcess_MakeLeftHanded |
            aiProcess_FlipWindingOrder |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FindInvalidData |
            aiProcess_ImproveCacheLocality |
            aiProcess_OptimizeMeshes |
            // aiProcess_OptimizeGraph,
            0;

        aiPropertyStore *propertyStore = aiCreatePropertyStore();
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_GLOB_MEASURE_TIME, 1);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_RVC_FLAGS, notRequiredComponents);
        aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);
        // aiSetImportPropertyInteger(propertyStore, AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 65535);

        auto filePath = context->findDataPath(FileSystem::CreatePath("physics", parameters.sourceName));
        context->log(Context::Info, "Loading: {}", filePath.getString());
        auto inputScene = aiImportFileExWithProperties(filePath.getString().data(), importFlags, nullptr, propertyStore);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to load scene with Assimp");
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_Triangulate);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to apply post processing with Assimp");
            return -__LINE__;
        }

        inputScene = aiApplyPostProcessing(inputScene, aiProcess_SplitLargeMeshes);
        if (inputScene == nullptr)
        {
            context->log(Context::Error, "Unable to apply post processing with Assimp");
            return -__LINE__;
        }

        if (!inputScene->HasMeshes())
        {
            context->log(Context::Error, "Scene has no meshes");
            return -__LINE__;
        }

        if (!inputScene->HasMaterials())
        {
            context->log(Context::Error, "Exporting to model requires materials in scene");
            return -__LINE__;
        }

        auto texturesPath(context->findDataPath("textures").getString());
        auto engineIndex = texturesPath.find("gek engine");
        if (engineIndex != std::string::npos)
        {
            // skip hard drive location, jump to known engine structure
            texturesPath = texturesPath.substr(engineIndex);
        }

        std::function<FileSystem::Path(const char *, FileSystem::Path const &)> removeRoot = [](const char *location, FileSystem::Path const &path) -> FileSystem::Path
        {
            auto parentPath = path.getParentPath();
            while (parentPath.isDirectory() && parentPath != path.getRootPath())
            {
                if (parentPath.getFileName() == location)
                {
                    return path.lexicallyRelative(parentPath);
                }
                else
                {
                    parentPath = parentPath.getParentPath();
                }
            };

            return path;
        };

        std::map<std::string, std::string> albedoToMaterialMap;
        std::function<bool(FileSystem::Path const &)> findMaterials;
        findMaterials = [&](FileSystem::Path const &filePath) -> bool
        {
            if (filePath.getExtension() != ".json")
            {
                return true;
            }

            JSON::Object materialNode = JSON::Load(filePath);
            auto &shaderNode = materialNode["shader"];
            auto &dataNode = shaderNode["data"];
            auto &albedoNode = dataNode["albedo"];
            auto albedoFile = JSON::Value(albedoNode, "file", String::Empty);
            auto albedoPath = removeRoot("textures", context->findDataPath(albedoFile));

            context->log(Context::Info, "Found material: {}, , with albedo: {}", filePath.getString(), albedoFile);
            albedoToMaterialMap[String::GetLower(albedoFile)] = String::GetLower(removeRoot("materials", filePath).getString());
            return true;
        };

        context->findDataFiles("materials", findMaterials);
        if (albedoToMaterialMap.empty())
        {
            context->log(Context::Error, "Unable to locate any materials");
            return -__LINE__;
        }

        auto findMaterialForMesh = [&](const FileSystem::Path &sourceName, std::string diffuseName) -> std::string
        {
            context->log(Context::Info, "> Searching for : {}, {}", diffuseName, (filePath / diffuseName).getString());

            FileSystem::Path albedoPath = FileSystem::GetCanonicalPath(filePath / diffuseName);
            if (!albedoPath.isFile())
            {
                albedoPath = FileSystem::Path(diffuseName).lexicallyRelative("textures");
                albedoPath = context->findDataPath(FileSystem::Path("textures") / filePath.withoutExtension().getFileName() / albedoPath);
            }

            if (albedoPath.isFile())
            {
                albedoPath = removeRoot("textures", albedoPath);
                auto albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.withoutExtension().getString()));
                if (albedoSearch == std::end(albedoToMaterialMap))
                {
                    albedoSearch = albedoToMaterialMap.find(String::GetLower(albedoPath.getString()));
                }

                if (albedoSearch != std::end(albedoToMaterialMap))
                {
                    context->log(Context::Info, "  Found material for albedo: {}, belongs to {}", albedoPath.getString(), albedoSearch->second);
                    return albedoSearch->second;
                }
            }

            context->log(Context::Error, "! Unable to find material for albedo: {}, {}", diffuseName, albedoPath.getString());
            return "";
        };

        Model model;
        aiMatrix4x4 identity;
        if (!GetModels(context.get(), parameters, inputScene, inputScene->mRootNode, identity, model, findMaterialForMesh))
        {
            return -__LINE__;
        }

        if (!SanitizeTreeModel(context.get(), model, parameters.sourceName))
        {
            return -__LINE__;
        }

        aiReleasePropertyStore(propertyStore);
        aiReleaseImport(inputScene);

        context->log(Context::Info, "- Num. Meshes: {}", model.meshList.size());
        context->log(Context::Info, "- Size: Minimum[{}, {}, {}]", model.boundingBox.minimum.x, model.boundingBox.minimum.y, model.boundingBox.minimum.z);
        context->log(Context::Info, "- Size: Maximum[{}, {}, {}]", model.boundingBox.maximum.x, model.boundingBox.maximum.y, model.boundingBox.maximum.z);

        auto outputPath(filePath.withoutExtension().withExtension(".gek"));
        context->log(Context::Info, "Writing: {}", outputPath.getString());
        outputPath.getParentPath().createChain();

        std::ofstream file;
        file.open(outputPath.getString().data(), std::ios::out | std::ios::binary);
        if (file.is_open())
        {
            std::set<std::string> materialList;
            for (auto &mesh : model.meshList)
            {
                materialList.insert(mesh.material);
            }

            if (materialList.size() > std::numeric_limits<uint32_t>::max() || model.meshList.size() > std::numeric_limits<uint32_t>::max())
            {
                context->log(Context::Error, "Tree physics model exceeds format limits: {}", parameters.sourceName);
                return -__LINE__;
            }

            Header header;
            header.materialCount = static_cast<uint32_t>(materialList.size());
            header.meshCount = static_cast<uint32_t>(model.meshList.size());
            FileSystem::Write(file, &header, 1);
            for (auto const &material : materialList)
            {
                Header::Material materialHeader;
                std::strncpy(materialHeader.name, material.data(), 63);
                FileSystem::Write(file, &materialHeader, 1);
            }

            for (auto const &mesh : model.meshList)
            {
                context->log(Context::Info, "Material: {}", mesh.material);
                context->log(Context::Info, "Num. Points: {}", mesh.pointList.size());
                context->log(Context::Info, "Num. Faces: {}", mesh.faceList.size());

                auto materialSearch = materialList.find(mesh.material);

                Header::Mesh meshHeader;
                if (mesh.faceList.size() > std::numeric_limits<uint32_t>::max() || mesh.pointList.size() > std::numeric_limits<uint32_t>::max())
                {
                    context->log(Context::Error, "Mesh exceeds format limits while writing tree physics model: {}", parameters.sourceName);
                    return -__LINE__;
                }

                meshHeader.materialIndex = static_cast<uint32_t>(std::distance(std::begin(materialList), materialSearch));
                meshHeader.faceCount = static_cast<uint32_t>(mesh.faceList.size());
                meshHeader.pointCount = static_cast<uint32_t>(mesh.pointList.size());
                FileSystem::Write(file, &meshHeader, 1);
                FileSystem::Write(file, mesh.faceList.data(), meshHeader.faceCount);
                FileSystem::Write(file, mesh.pointList.data(), meshHeader.pointCount);
            }

            file.close();
        }
        else
        {
            context->log(Context::Error, "Unable to create output file");
        }
    }

    return 0;
}
#include "API/Engine/Core.hpp"
#include "API/Engine/Editor.hpp"
#include "API/Engine/Entity.hpp"
#include "API/Engine/Population.hpp"
#include "API/Engine/Processor.hpp"
#include "API/Engine/Visualizer.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Model/Base.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Physics/MatrixUtil.hpp"
#include "GEK/Physics/StaticBody.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <dCollision/ndContactNotify.h>
#include <dCollision/ndShapeCompound.h>
#include <future>
#include <map>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

namespace Gek
{
    namespace Physics
    {
        extern BodyPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, World *World, Plugin::Entity *const entity);
        extern BodyPtr createRigidBody(World *world, Plugin::Entity *const entity);

        class BufferReader
        {
          private:
            uint8_t *buffer = nullptr;
            size_t size = 0;
            size_t index = 0;

          public:
            BufferReader(uint8_t *buffer, size_t size)
                : buffer(buffer), size(size)
            {
            }

            template <typename TYPE>
            bool canRead(uint32_t count = 1) const
            {
                size_t readSize = sizeof(TYPE) * static_cast<size_t>(count);
                return (index + readSize) <= size;
            }

            template <typename TYPE>
            TYPE *read(uint32_t count = 1)
            {
                if (!canRead<TYPE>(count))
                {
                    return nullptr;
                }

                TYPE *data = (TYPE *)(buffer + index);
                index += (sizeof(TYPE) * count);
                return data;
            }
        };

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
        , public Plugin::Processor, public World
        {
          public:
            struct Header
            {
                uint32_t identifier = 0;
                uint16_t type = 0;
                uint16_t version = 0;
            };

            struct HullHeader : public Header
            {
                uint32_t pointCount;
            };

            struct TreeHeader : public Header
            {
                struct Material
                {
                    char name[64];
                };

                struct Mesh
                {
                    uint32_t materialIndex;
                    uint32_t faceCount;
                    uint32_t pointCount;
                };

                struct Face
                {
                    int32_t indices[3];
                };

                uint32_t materialCount;
                uint32_t meshCount;
            };

            struct Vertex
            {
                Math::Float3 position;
                Math::Float2 texCoord;
                Math::Float3 normal;
            };

            struct Material
            {
                uint32_t firstVertex;
                uint32_t firstIndex;
                uint32_t indexCount;
            };

            class ContactNotify : public ndContactNotify
            {
                Processor *processor;

              public:
                ContactNotify(ndScene *scene, Processor *processor)
                    : ndContactNotify(scene), processor(processor) {}
                void OnContactCallback(const ndContact *const contact, ndFloat32 timestep) const override
                {
                    const auto &points = contact->GetContactPoints();
                    using NodeType = ndList<ndContactMaterial, ndContainersFreeListAlloc<ndContactMaterial>>::ndNode;
                    for (NodeType *node = points.GetFirst(); node; node = node->GetNext())
                    {
                        const ndContactMaterial &cp = node->GetInfo();
                        Plugin::Entity *entity0 = nullptr;
                        Plugin::Entity *entity1 = nullptr;
                        ndBodyKinematic *body0 = (ndBodyKinematic *)contact->GetBody0();
                        ndBodyKinematic *body1 = (ndBodyKinematic *)contact->GetBody1();
                        for (auto &pair : processor->entityBodyMap)
                        {
                            if (pair.second && pair.second->getAsNewtonBody() == body0)
                                entity0 = pair.first;
                            if (pair.second && pair.second->getAsNewtonBody() == body1)
                                entity1 = pair.first;
                        }
                        if (!entity0 || !entity1)
                            continue;
                        Math::Float3 position(cp.m_point.m_x, cp.m_point.m_y, cp.m_point.m_z);
                        Math::Float3 normal(cp.m_normal.m_x, cp.m_normal.m_y, cp.m_normal.m_z);
                        processor->onCollision(entity0, position, normal, entity1);
                    }
                }
            };

            class NewtonWorld : public ndWorld
            {
              public:
                NewtonWorld(Processor *processor)
                {
                    SetContactNotify(new ContactNotify(this->GetScene(), processor));
                }

                ~NewtonWorld()
                {
                }
            };

          private:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
            Plugin::Visualizer *renderer = nullptr;
            Edit::Events *events = nullptr;

            tbb::concurrent_vector<Surface> surfaceList;
            tbb::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            NewtonWorld *newtonWorld = nullptr;
            ThreadPool loadPool;

            tbb::concurrent_unordered_map<Plugin::Entity *, Physics::Body *> entityBodyMap;
            tbb::concurrent_unordered_map<Hash, std::shared_future<ndShape *>> shapeFutureMap;

          public:
            Processor(Context * context, Plugin::Core * core)
                : ContextRegistration(context), core(core), population(core->getPopulation()), renderer(core->getVisualizer()), loadPool(5)
            {
                assert(core);
                assert(population);
                assert(renderer);

                core->onInitialized.connect(this, &Processor::onInitialized);
                core->onShutdown.connect(this, &Processor::onShutdown);
                population->onReset.connect(this, &Processor::onReset);
                population->onEntityCreated.connect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.connect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.connect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.connect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].connect(this, &Processor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Processor::onShowUserInterface);

                // Do NOT call onReset() here. Call initialize() after construction.
            }

            // Call this after construction to safely initialize NewtonWorld and threads
            void initialize()
            {
                onReset();
            }

            void clear(void)
            {
                if (newtonWorld)
                {
                    newtonWorld->Sync();

                    surfaceList.clear();
                    surfaceIndexMap.clear();

                    newtonWorld->CleanUp();

                    entityBodyMap.clear();
                    shapeFutureMap.clear();

                    delete newtonWorld;
                    newtonWorld = nullptr;
                }
                else
                {
                }
            }

            Task scheduleLoadShape(std::shared_ptr<std::promise<ndShape *>> promise, Components::Model const &modelComponent)
            {
                co_await loadPool.schedule();

                ndShape *shape = nullptr;
                if (modelComponent.name == "#cube")
                {
                    getContext()->log(Context::Info, "Loading box shape for model: {}", modelComponent.name);
                    shape = new ndShapeBox(1.0f, 1.0f, 1.0f);
                }
                else if (modelComponent.name == "#sphere")
                {
                    getContext()->log(Context::Info, "Loading sphere shape for model: {}", modelComponent.name);
                    shape = new ndShapeSphere(1.0f);
                }
                else
                {
                    auto filePath = getContext()->findDataPath(FileSystem::CreatePath("physics", modelComponent.name).withExtension(".gek"));
                    getContext()->log(Context::Info, "Loading physics model from file: {}", filePath.getFileName());
                    std::vector<uint8_t> buffer(FileSystem::Load(filePath));
                    if (buffer.size() < sizeof(Header))
                    {
                        getContext()->log(Context::Error, "File too small to be physics model: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    BufferReader reader(buffer.data(), buffer.size());
                    Header *header = reader.read<Header>(0);
                    if (!header)
                    {
                        getContext()->log(Context::Error, "Unable to read physics header: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->identifier != *(uint32_t *)"GEKX")
                    {
                        getContext()->log(Context::Error, "Unknown model file identifier encountered: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->version != 3)
                    {
                        getContext()->log(Context::Error, "Unsupported model version encountered (requires: 3, has: {}): {}", header->version, modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->type == 1)
                    {
                        getContext()->log(Context::Info, "Loading convex hull for static scene: {}", modelComponent.name);
                        HullHeader *hullHeader = reader.read<HullHeader>();
                        if (!hullHeader)
                        {
                            getContext()->log(Context::Error, "Unable to read convex hull header: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        Math::Float3 *points = reader.read<Math::Float3>(hullHeader->pointCount);
                        if (!points)
                        {
                            getContext()->log(Context::Error, "Invalid convex hull point data in physics model: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        shape = new ndShapeConvexHull(hullHeader->pointCount, sizeof(Math::Float3), 0.0f, points->data);
                    }
                    else if (header->type == 2)
                    {
                        getContext()->log(Context::Info, "Loading tree mesh for static scene: {}", modelComponent.name);
                        TreeHeader *treeHeader = reader.read<TreeHeader>();
                        if (!treeHeader)
                        {
                            getContext()->log(Context::Error, "Unable to read tree mesh header: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        // Read materials
                        std::vector<std::string> materialNames;
                        for (uint32_t i = 0; i < treeHeader->materialCount; ++i)
                        {
                            TreeHeader::Material *mat = reader.read<TreeHeader::Material>();
                            if (!mat)
                            {
                                getContext()->log(Context::Error, "Invalid material data in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            materialNames.push_back(std::string(mat->name));
                        }

                        // Read meshes
                        std::vector<TreeHeader::Mesh> meshes;
                        for (uint32_t i = 0; i < treeHeader->meshCount; ++i)
                        {
                            TreeHeader::Mesh *mesh = reader.read<TreeHeader::Mesh>();
                            if (!mesh)
                            {
                                getContext()->log(Context::Error, "Invalid mesh header in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            meshes.push_back(*mesh);
                        }

                        // Collect all valid triangles into a flat list first, then split into
                        // chunks. Newton's CalculateAdjacent calls ForAllSectors with the full
                        // scene AABB, which uses an ndFixSizeArray<..., 512> traversal stack.
                        // Newton uses fixed-size traversal stacks of 512 entries in several
                        // paths, with push logic that can write one past end at exactly 512.
                        // Keep a generous margin below that limit.
                        static constexpr uint32_t MaxFacesPerChunk = 240;

                        struct Triangle
                        {
                            ndVector v[3];
                        };
                        std::vector<Triangle> allTriangles;
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

                        struct QuantizedPoint
                        {
                            long long x;
                            long long y;
                            long long z;
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

                        std::map<std::array<long long, 9>, size_t> triangleFaceMap;
                        std::map<std::array<long long, 6>, std::vector<size_t>> edgeFaceMap;
                        std::vector<std::array<double, 3>> triangleNormals;

                        for (auto &mesh : meshes)
                        {
                            TreeHeader::Face *faces = reader.read<TreeHeader::Face>(mesh.faceCount);
                            Math::Float3 *meshPoints = reader.read<Math::Float3>(mesh.pointCount);
                            if (!faces || !meshPoints)
                            {
                                getContext()->log(Context::Error, "Invalid face/point data in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            for (uint32_t f = 0; f < mesh.faceCount; ++f)
                            {
                                int32_t i0 = faces[f].indices[0];
                                int32_t i1 = faces[f].indices[1];
                                int32_t i2 = faces[f].indices[2];

                                if (i0 < 0 || i1 < 0 || i2 < 0 ||
                                    static_cast<uint32_t>(i0) >= mesh.pointCount ||
                                    static_cast<uint32_t>(i1) >= mesh.pointCount ||
                                    static_cast<uint32_t>(i2) >= mesh.pointCount)
                                {
                                    ++invalidFaceCount;
                                    continue;
                                }

                                if ((i0 == i1) || (i1 == i2) || (i2 == i0))
                                {
                                    ++duplicateIndexFaceCount;
                                    continue;
                                }

                                Math::Float3 const &p0 = meshPoints[i0];
                                Math::Float3 const &p1 = meshPoints[i1];
                                Math::Float3 const &p2 = meshPoints[i2];

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

                                Triangle tri;
                                tri.v[0] = ndVector(p0.x, p0.y, p0.z, 0.0f);
                                tri.v[1] = ndVector(p1.x, p1.y, p1.z, 0.0f);
                                tri.v[2] = ndVector(p2.x, p2.y, p2.z, 0.0f);
                                size_t const triIndex = allTriangles.size();
                                allTriangles.push_back(tri);
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

                        if (!allTriangles.empty())
                        {
                            std::vector<bool> rejected(allTriangles.size(), false);
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

                            if ((nonManifoldFaceCount > 0) || (invalidAdjacencyFaceCount > 0))
                            {
                                size_t rejectedFaceCount = 0;
                                for (bool const isRejected : rejected)
                                {
                                    if (isRejected)
                                    {
                                        ++rejectedFaceCount;
                                    }
                                }

                                std::vector<Triangle> manifoldTriangles;
                                manifoldTriangles.reserve(allTriangles.size() - std::min(rejectedFaceCount, allTriangles.size()));
                                for (size_t i = 0; i < allTriangles.size(); ++i)
                                {
                                    if (!rejected[i])
                                    {
                                        manifoldTriangles.push_back(allTriangles[i]);
                                    }
                                }
                                allTriangles.swap(manifoldTriangles);
                            }
                        }

                        if (invalidFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} invalid faces while loading tree physics model: {}", invalidFaceCount, modelComponent.name);
                        }

                        if (duplicateIndexFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} duplicate-index faces while loading tree physics model: {}", duplicateIndexFaceCount, modelComponent.name);
                        }

                        if (nonFiniteFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} non-finite faces while loading tree physics model: {}", nonFiniteFaceCount, modelComponent.name);
                        }

                        if (weldedEdgeFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} pre-weld-collapsed faces while loading tree physics model: {}", weldedEdgeFaceCount, modelComponent.name);
                        }

                        if (weldedDegenerateFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} weld-space degenerate faces while loading tree physics model: {}", weldedDegenerateFaceCount, modelComponent.name);
                        }

                        if (duplicateWeldedTriangleFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} duplicate weld-space triangles while loading tree physics model: {}", duplicateWeldedTriangleFaceCount, modelComponent.name);
                        }

                        if (nonManifoldFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} non-manifold welded faces while loading tree physics model: {}", nonManifoldFaceCount, modelComponent.name);
                        }

                        if (invalidAdjacencyFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} invalid-adjacency faces while loading tree physics model: {}", invalidAdjacencyFaceCount, modelComponent.name);
                        }

                        if ((nonManifoldFaceCount > 0) || (duplicateWeldedTriangleFaceCount > 0) || (invalidAdjacencyFaceCount > 0))
                        {
                            getContext()->log(Context::Error, "Rejected tree physics model due to invalid welded topology or adjacency: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        if (shortEdgeFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} short-edge faces while loading tree physics model: {}", shortEdgeFaceCount, modelComponent.name);
                        }

                        if (sliverFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} sliver faces while loading tree physics model: {}", sliverFaceCount, modelComponent.name);
                        }

                        if (degenerateFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} degenerate faces while loading tree physics model: {}", degenerateFaceCount, modelComponent.name);
                        }

                        uint32_t totalFaces = static_cast<uint32_t>(allTriangles.size());
                        if (totalFaces == 0)
                        {
                            getContext()->log(Context::Error, "No valid faces in tree physics model: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        uint32_t chunkCount = (totalFaces + MaxFacesPerChunk - 1) / MaxFacesPerChunk;
                        getContext()->log(Context::Info, "Building BVH for {}: {} faces in {} chunk(s) of max {}", modelComponent.name, totalFaces, chunkCount, MaxFacesPerChunk);

                        auto *compound = new ndShapeCompound();
                        compound->BeginAddRemove();

                        for (uint32_t chunk = 0; chunk < chunkCount; ++chunk)
                        {
                            uint32_t start = chunk * MaxFacesPerChunk;
                            uint32_t end = std::min(start + MaxFacesPerChunk, totalFaces);

                            ndPolygonSoupBuilder builder;
                            builder.Begin();
                            for (uint32_t f = start; f < end; ++f)
                            {
                                builder.AddFace(&allTriangles[f].v[0], 3, 0);
                            }
                            builder.End(false);

                            compound->AddCollision(new ndShapeInstance(new ndShapeStatic_bvh(builder)));
                        }

                        compound->EndAddRemove();
                        shape = compound;
                    }
                    else
                    {
                        getContext()->log(Context::Error, "Unsupported model type encountered: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }
                }

                if (shape)
                {
                    getContext()->log(Context::Info, "Physics shape successfully loaded: {}", modelComponent.name);
                    // AddRef once here so the cache holds a permanent reference.
                    // ndShape starts with refCount=0; the ndShapeInstance copy constructor
                    // deep-copies ndShapeCompound and Releases the original, which would
                    // drop refCount back to 0 and delete the shape after the first use.
                    // Holding a permanent ref prevents that and keeps the cached pointer valid.
                    shape->AddRef();
                    promise->set_value(shape);
                }
                else
                {
                    getContext()->log(Context::Error, "Unable to create physics shape: {}", modelComponent.name);
                    promise->set_value(nullptr);
                }
            }

            ndShape *loadShape(Components::Model const &modelComponent)
            {
                auto hash = GetHash(modelComponent.name);
                auto promise = std::make_shared<std::promise<ndShape *>>();
                auto future = promise->get_future().share();
                auto shapeInsert = shapeFutureMap.insert(std::make_pair(hash, future));
                if (shapeInsert.second)
                {
                    scheduleLoadShape(promise, modelComponent);
                }

                auto shapeFuture = shapeInsert.first->second;
                ndShape *shape = shapeFuture.get();
                return shape;
            }

            // concurrency::critical_section criticalSection;
            void addEntity(Plugin::Entity *const entity)
            {
                getContext()->log(Context::Info, "Adding entity to physics processor");

                BodyPtr body;
                if (entity->hasComponent<Components::Transform>())
                {
                    // Handle static scene geometry (Model + Scene, no Physical)
                    if (entity->hasComponents<Components::Model, Components::Scene>() && !entity->hasComponent<Components::Physical>())
                    {
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                        auto shape = loadShape(modelComponent);
                        if (shape)
                        {
                            auto &transformComponent = entity->getComponent<Components::Transform>();
                            auto staticBody = std::make_unique<StaticBody>(transformComponent.getMatrix(), ndShapeInstance(shape));
                            if (newtonWorld)
                            {
                                newtonWorld->AddBody(staticBody->getAsNewtonBody());
                            }

                            entityBodyMap[entity] = staticBody.release();
                        }
                    }
                    // Handle dynamic/kinematic bodies
                    else if (entity->hasComponents<Components::Physical>())
                    {
                        auto &physicalComponent = entity->getComponent<Components::Physical>();
                        if (entity->hasComponent<Components::Player>())
                        {
                            body = createPlayerBody(core, population, this, entity);
                        }
                        else if (entity->hasComponent<Components::Model>())
                        {
                            auto const &modelComponent = entity->getComponent<Components::Model>();
                            auto shape = loadShape(modelComponent);
                            if (shape)
                            {
                                body = createRigidBody(this, entity);
                                if (body)
                                {
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                                }
                            }
                        }
                    }
                }

                if (body)
                {
                    if (newtonWorld)
                    {
                        ndSharedPtr<ndBody> sharedBody(body->getAsNewtonBody());
                        auto &transformComponent = entity->getComponent<Components::Transform>();
                        sharedBody->SetMatrix(MakeNewtonMatrix(transformComponent.getMatrix()));
                        newtonWorld->AddBody(sharedBody);
                    }

                    entityBodyMap[entity] = body.release();
                }
            }

            void removeEntity(Plugin::Entity *const entity)
            {
                auto entitySearch = entityBodyMap.find(entity);
                if (entitySearch != std::end(entityBodyMap))
                {
                    newtonWorld->RemoveBody(entitySearch->second->getAsNewtonBody());
                    entityBodyMap.unsafe_erase(entitySearch);
                }
            }

            // Plugin::Core
            void onInitialized(void)
            {
                core->listProcessors([&](Plugin::Processor *processor) -> void
                                     {
                    auto castCheck = dynamic_cast<Edit::Events *>(processor);
                    if (castCheck)
                    {
                        (events = castCheck)->onModified.connect(this, &Processor::onModified);
                    } });
            }

            void onShutdown(void)
            {
                if (events)
                {
                    events->onModified.disconnect(this, &Processor::onModified);
                }

                renderer->onShowUserInterface.disconnect(this, &Processor::onShowUserInterface);
                population->onReset.disconnect(this, &Processor::onReset);
                population->onEntityCreated.disconnect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.disconnect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.disconnect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.disconnect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].disconnect(this, &Processor::onUpdate);

                clear();
            }

            // Plugin::Editor Slots
            void onModified(Plugin::Entity *const entity, Hash type)
            {
                auto bodySearch = entityBodyMap.find(entity);
                if (bodySearch == std::end(entityBodyMap))
                {
                    return;
                }

                auto body = bodySearch->second;
                if (type == Components::Transform::GetIdentifier())
                {
                    auto entitySearch = entityBodyMap.find(entity);
                    if (entitySearch != std::end(entityBodyMap))
                    {
                        auto const &transformComponent = entity->getComponent<Components::Transform>();
                        auto matrix(transformComponent.getMatrix());
                        body->getAsNewtonBody()->SetMatrix(MakeNewtonMatrix(matrix));
                    }
                }
                else if (type == Components::Model::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Physical>())
                    {
                        auto const &physicalComponent = entity->getComponent<Components::Physical>();
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                        auto shape = loadShape(modelComponent);
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                    }
                }
                else if (type == Components::Physical::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Model>())
                    {
                        auto const &physicalComponent = entity->getComponent<Components::Physical>();
                        auto &shapeInstance = body->getAsNewtonBody()->GetAsBodyDynamic()->GetCollisionShape();
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, shapeInstance);
                    }
                }
            }

            // Plugin::Core Slots
            void onShowUserInterface(void)
            {
            }

            // Plugin::Population Slots
            void onReset(void)
            {
                clear();
                newtonWorld = new NewtonWorld(this);

                newtonWorld->Sync();
                newtonWorld->SetSubSteps(2);
                newtonWorld->SetSolverIterations(1);
                newtonWorld->SetThreadCount(4);
                // newtonWorld->SelectSolver(m_solverMode);
            }

            void onEntityCreated(Plugin::Entity *const entity)
            {
                addEntity(entity);
            }

            void onEntityDestroyed(Plugin::Entity *const entity)
            {
                removeEntity(entity);
            }

            void onComponentAdded(Plugin::Entity *const entity)
            {
                addEntity(entity);
            }

            void onComponentRemoved(Plugin::Entity *const entity)
            {
                if (!entity->hasComponents<Components::Transform, Components::Physical>())
                {
                    removeEntity(entity);
                }
            }

            void onUpdate(float frameTime)
            {
                bool editorActive = core->getOption("editor", "active", false);
                if (frameTime > 0.0f && !editorActive)
                {
                    static constexpr float StepTime = (1.0f / 60.0f);
                    while (frameTime > 0.0f)
                    {
                        if (newtonWorld)
                        {
                            newtonWorld->Update(StepTime);
                            newtonWorld->Sync();
                        }

                        frameTime -= StepTime;
                    };
                }
            }

            // Newton::World
            Math::Float3 getGravity(Math::Float3 const *position)
            {
                const Math::Float3 DefaultGravity(0.0f, -32.174f, 0.0f);

                auto localGravity = DefaultGravity;
                if (position)
                {
                }

                return localGravity;
            }

            uint32_t loadSurface(std::string const &surfaceName)
            {
                uint32_t surfaceIndex = 0;

                auto hash = GetHash(surfaceName);
                auto surfaceSearch = surfaceIndexMap.find(hash);
                if (surfaceSearch != std::end(surfaceIndexMap))
                {
                    surfaceIndex = surfaceSearch->second;
                }
                else
                {
                    surfaceIndexMap[hash] = 0;

                    JSON::Object materialNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("materials", surfaceName).withExtension(".json")));
                    auto surfaceNode = materialNode["surface"];
                    if (surfaceNode.is_object())
                    {
                        Surface surface;
                        surface.ghost = JSON::Value(surfaceNode, "ghost", surface.ghost);
                        surface.staticFriction = JSON::Value(surfaceNode, "static_friction", surface.staticFriction);
                        surface.kineticFriction = JSON::Value(surfaceNode, "kinetic_friction", surface.kineticFriction);
                        surface.elasticity = JSON::Value(surfaceNode, "elasticity", surface.elasticity);
                        surface.softness = JSON::Value(surfaceNode, "softness", surface.softness);

                        surfaceIndex = static_cast<uint32_t>(surfaceList.size());
                        surfaceList.push_back(surface);
                        surfaceIndexMap[hash] = surfaceIndex;
                    }
                }

                return surfaceIndex;
            }

            const Surface &getSurface(uint32_t surfaceIndex) const
            {
                static const Surface DefaultSurface;
                return (surfaceIndex >= surfaceList.size() ? DefaultSurface : surfaceList[surfaceIndex]);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Processor)
    }; // namespace Physics
}; // namespace Gek

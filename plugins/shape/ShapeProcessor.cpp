﻿#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\ThreadPool.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Color.h"
#include "GEK\Shape\Base.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <ppl.h>
#include <future>
#include <algorithm>
#include <memory>
#include <array>
#include <map>

namespace Gek
{
#ifdef _ENCODE_SPHEREMAP
    Math::Float2 encodeNormal(Math::Float3 normal)
    {
        Math::Float2 encoded(normal.x, normal.y);
        return encoded.getNormal() * std::sqrt(normal.z * 0.5f + 0.5f);
    }
#elif _ENCODE_OCTAHEDRON
    Math::Float2 octahedronWrap(Math::Float2 value)
    {
        return (1.0f - std::abs(value)) * (value >= 0.0f ? 1.0f : -1.0f);
    }

    Math::Float2 encodeNormal((Math::Float3 normal)
    {
        normal /= (std::abs(normal.x) + std::abs(normal.y) + std::abs(normal.z));

        Math::Float2 encoded(normal.x, normal.y);
        encoded = normal.z >= 0.0f ? encoded : OctWrap(encoded);
        encoded = encoded * 0.5f + 0.5f;
        return encoded;
    }
#else
    Math::Float2 encodeNormal(Math::Float3 normal)
    {
        return Math::Float2(normal.y, normal.z);
    }
#endif

    struct Vertex
    {
        Math::Float3 position;
        Math::Float2 texCoord;
        Math::Float3 normal;

        Vertex(void)
        {
        }

        Vertex(const Math::Float3 &position)
            : position(position)
            , normal(position)
            , texCoord(encodeNormal(position))
        {
        }

        Vertex(const Math::Float3 &position, const Math::Float2 &texCoord)
            : position(position)
            , normal(position)
            , texCoord(texCoord)
        {
        }
    };

    class GeoSphere
    {
    private:
        struct Edge
        {
            bool split; // whether this edge has been split
            std::array <size_t, 2> vertices; // the two endpoint vertex indices
            size_t splitVertex; // the split vertex index
            std::array <size_t, 2> splitEdges; // the two edges after splitting - they correspond to the "vertices" vector

            Edge(size_t v0, size_t v1)
                : vertices{ v0, v1 }
            {
                split = false;
            }

            size_t getSplitEdge(size_t vertex)
            {
                if (vertex == vertices[0])
                {
                    return splitEdges[0];
                }
                else if (vertex == vertices[1])
                {
                    return splitEdges[1];
                }
                else
                {
                    return 0;
                }
            }
        };

        struct Triangle
        {
            std::array <size_t, 3> vertices; // the vertex indices
            std::array <size_t, 3> edges; // the edge indices

            Triangle(size_t v0, size_t v1, size_t v2, size_t e0, size_t e1, size_t e2)
                : vertices{ v0, v1, v2 }
                , edges{ e0, e1, e2 }
            {
            }
        };

        std::vector<Vertex> vertices;
        std::vector<Edge> edges;
        std::vector<Edge> newEdges;
        std::vector<Triangle> triangles;
        std::vector<Triangle> newTriangles;

        std::vector <uint16_t> indices;
        float inscriptionRadiusMultiplier;

        void splitEdge(Edge & e)
        {
            if (e.split)
            {
                return;
            }

            e.splitVertex = vertices.size();
            vertices.push_back(Vertex((Math::Float3(0.5f) * (vertices[e.vertices[0]].position + vertices[e.vertices[1]].position)).getNormal()));
            e.splitEdges = { newEdges.size(), newEdges.size() + 1 };
            newEdges.push_back(Edge(e.vertices[0], e.splitVertex));
            newEdges.push_back(Edge(e.splitVertex, e.vertices[1]));
            e.split = true;
        }

        void subdivideTriangle(const Triangle & t)
        {
            //    0
            //    /\
            //  0/  \2
            //  /____\
            // 1  1   2
            size_t edge0 = t.edges[0];
            size_t edge1 = t.edges[1];
            size_t edge2 = t.edges[2];
            size_t vert0 = t.vertices[0];
            size_t vert1 = t.vertices[1];
            size_t vert2 = t.vertices[2];
            splitEdge(edges[edge0]);
            splitEdge(edges[edge1]);
            splitEdge(edges[edge2]);
            size_t edge01 = newEdges.size();
            size_t edge12 = newEdges.size() + 1;
            size_t edge20 = newEdges.size() + 2;
            newEdges.push_back(Edge(edges[edge0].splitVertex, edges[edge1].splitVertex));
            newEdges.push_back(Edge(edges[edge1].splitVertex, edges[edge2].splitVertex));
            newEdges.push_back(Edge(edges[edge2].splitVertex, edges[edge0].splitVertex));

            // important: we push the "center" triangle first
            // this is so a center-most triangle is guaranteed to be at index 0 of the final object
            newTriangles.push_back(Triangle(edges[edge0].splitVertex, edges[edge1].splitVertex, edges[edge2].splitVertex, edge01, edge12, edge20));

            newTriangles.push_back(Triangle(vert0, edges[edge0].splitVertex, edges[edge2].splitVertex, edges[edge0].getSplitEdge(vert0), edge20, edges[edge2].getSplitEdge(vert0)));

            newTriangles.push_back(Triangle(edges[edge0].splitVertex, vert1, edges[edge1].splitVertex, edges[edge0].getSplitEdge(vert1), edges[edge1].getSplitEdge(vert1), edge01));

            newTriangles.push_back(Triangle(edges[edge2].splitVertex, edges[edge1].splitVertex, vert2, edge12, edges[edge1].getSplitEdge(vert2), edges[edge2].getSplitEdge(vert2)));
        }

        void subdivide()
        {
            size_t trianglesCount = triangles.size();
            for (size_t i = 0; i < trianglesCount; ++i)
            {
                subdivideTriangle(triangles[i]);
            }

            edges.swap(newEdges);
            triangles.swap(newTriangles);
            newEdges.clear();
            newTriangles.clear();
        }

        float computeInscriptionRadiusMultiplier() const
        {
            // all triangles have 3 points with norm 1
            // this means for each triangle, the point on the plane
            // with the smallest norm is the centroid (average) of
            // the three vertices. Thus, since all vertices have
            // norm 1, the largest triangle will have a centroid
            // with mean closest to 0. Each time we subdivide a triangle,
            // we get 3 triangles with 2 points "pushed out" (the surrounding
            // triangles) and one triangle with all 3 points "pushed out"
            // (the center triangle). Since pushing out a point makes the
            // triangle bigger, the biggest triangle must be the
            // center-most triangle - i.e. take the path of the center
            // triangle in each subdivision to get the biggest triangle.
            // In the subdivideTriangle() method, we ensured the center
            // triangle is always pushed to the new triangle array first -
            // thus, one of the center triangles must have index 0. Therefore
            // we compute the centroid of the triangle with index 0
            // and take its norm. If we divide by this value, "expand" the
            // sphere to contain a given radius rather than be contained by
            // a given radius. However, since division is slower, we'd rather
            // multiply, so we return the reciprocal.
            // 1/3 times the sum of the three vertices
            static const Math::Float3 oneThird(1.0f / 3.0f);
            Math::Float3 centroid(oneThird * (vertices[triangles[0].vertices[0]].position + vertices[triangles[0].vertices[1]].position + vertices[triangles[0].vertices[2]].position));
            return (1.0f / centroid.getLength());
        }

    public:
        GeoSphere()
        {
        }

        void generate(size_t subdivisions)
        {
            static const float PHI = 1.618033988749894848204586834365638f;

            static const Vertex initialVertices[] =
            {
                Vertex(Math::Float3(-1.0f,  0.0f,   PHI).getNormal()),  // 0
                Vertex(Math::Float3(1.0f,  0.0f,   PHI).getNormal()),  // 1
                Vertex(Math::Float3(0.0f,   PHI,  1.0f).getNormal()),  // 2
                Vertex(Math::Float3(-PHI,  1.0f,  0.0f).getNormal()),  // 3
                Vertex(Math::Float3(-PHI, -1.0f,  0.0f).getNormal()),  // 4
                Vertex(Math::Float3(0.0f,  -PHI,  1.0f).getNormal()),  // 5
                Vertex(Math::Float3(PHI, -1.0f,  0.0f).getNormal()),  // 6
                Vertex(Math::Float3(PHI,  1.0f,  0.0f).getNormal()),  // 7
                Vertex(Math::Float3(0.0f,   PHI, -1.0f).getNormal()),  // 8
                Vertex(Math::Float3(-1.0f,  0.0f,  -PHI).getNormal()),  // 9
                Vertex(Math::Float3(0.0f,  -PHI, -1.0f).getNormal()),  // 10
                Vertex(Math::Float3(1.0f,  0.0f,  -PHI).getNormal()),  // 11
            };

            static const Edge initialEdges[] =
            {
                Edge(0,  1),   // 0
                Edge(0,  2),   // 1
                Edge(0,  3),   // 2
                Edge(0,  4),   // 3
                Edge(0,  5),   // 4
                Edge(1,  2),   // 5
                Edge(2,  3),   // 6
                Edge(3,  4),   // 7
                Edge(4,  5),   // 8
                Edge(5,  1),   // 9
                Edge(5,  6),   // 10
                Edge(6,  1),   // 11
                Edge(1,  7),   // 12
                Edge(7,  2),   // 13
                Edge(2,  8),   // 14
                Edge(8,  3),   // 15
                Edge(3,  9),   // 16
                Edge(9,  4),   // 17
                Edge(4, 10),   // 18
                Edge(10,  5),   // 19
                Edge(10,  6),   // 20
                Edge(6,  7),   // 21
                Edge(7,  8),   // 22
                Edge(8,  9),   // 23
                Edge(9, 10),   // 24
                Edge(10, 11),   // 25
                Edge(6, 11),   // 26
                Edge(7, 11),   // 27
                Edge(8, 11),   // 28
                Edge(9, 11),   // 29
            };

            static const Triangle initialTriangles[] =
            {
                Triangle(0,  1,  2,  0,  5,  1),
                Triangle(0,  2,  3,  1,  6,  2),
                Triangle(0,  3,  4,  2,  7,  3),
                Triangle(0,  4,  5,  3,  8,  4),
                Triangle(0,  5,  1,  4,  9,  0),
                Triangle(1,  6,  7, 11, 21, 12),
                Triangle(1,  7,  2, 12, 13,  5),
                Triangle(2,  7,  8, 13, 22, 14),
                Triangle(2,  8,  3, 14, 15,  6),
                Triangle(3,  8,  9, 15, 23, 16),
                Triangle(3,  9,  4, 16, 17,  7),
                Triangle(4,  9, 10, 17, 24, 18),
                Triangle(4, 10,  5, 18, 19,  8),
                Triangle(5, 10,  6, 19, 20, 10),
                Triangle(5,  6,  1, 10, 11,  9),
                Triangle(6, 11,  7, 26, 27, 21),
                Triangle(7, 11,  8, 27, 28, 22),
                Triangle(8, 11,  9, 28, 29, 23),
                Triangle(9, 11, 10, 29, 25, 24),
                Triangle(10, 11,  6, 25, 26, 20),
            };

            vertices.clear();
            edges.clear();
            triangles.clear();

            size_t vertexCount = ARRAYSIZE(initialVertices);
            size_t edgeCount = ARRAYSIZE(initialEdges);
            size_t triangleCount = ARRAYSIZE(initialTriangles);

            // reserve space
            for (size_t i = 0; i < subdivisions; ++i)
            {
                vertexCount += edgeCount;
                edgeCount = edgeCount * 2 + triangleCount * 3;
                triangleCount *= 4;
            }

            vertices.reserve(vertexCount);
            edges.reserve(edgeCount);
            newEdges.reserve(edgeCount);
            triangles.reserve(triangleCount);
            newTriangles.reserve(triangleCount);

            vertices.assign(initialVertices, initialVertices + ARRAYSIZE(initialVertices));
            edges.assign(initialEdges, initialEdges + ARRAYSIZE(initialEdges));
            triangles.assign(initialTriangles, initialTriangles + ARRAYSIZE(initialTriangles));

            for (size_t i = 0; i < subdivisions; ++i)
            {
                subdivide();
            }

            inscriptionRadiusMultiplier = computeInscriptionRadiusMultiplier();

            // now we create the array of indices
            size_t trianglesCount = triangles.size();
            indices.reserve(trianglesCount * 3);
            for (size_t i = 0; i < trianglesCount; ++i)
            {
                indices.push_back((uint16_t)triangles[i].vertices[0]);
                indices.push_back((uint16_t)triangles[i].vertices[1]);
                indices.push_back((uint16_t)triangles[i].vertices[2]);
            }

            for (auto &vertex : vertices)
            {
                vertex.position *= 0.5f;
            }
        }

        ResourceHandle createVertexBuffer(const wchar_t *name, Plugin::Resources *resources)
        {
            uint8_t *data = (uint8_t *)vertices.data();
            uint32_t size = (vertices.size() * sizeof(Vertex));
            return resources->createBuffer(name, sizeof(Vertex), vertices.size(), Video::BufferType::Vertex, 0, std::vector<uint8_t>(data, data + size));
        }

        ResourceHandle createIndexBuffer(const wchar_t *name, Plugin::Resources *resources)
        {
            uint8_t *data = (uint8_t *)indices.data();
            uint32_t size = (indices.size() * sizeof(uint16_t));
            return resources->createBuffer(name, Video::Format::R16_UINT, indices.size(), Video::BufferType::Index, 0, std::vector<uint8_t>(data, data + size));
        }

        uint32_t getIndexCount(void) const
        {
            return indices.size();
        }
    };

    namespace Components
    {
        void Shape::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, type);
            saveParameter(componentData, L"parameters", parameters);
            saveParameter(componentData, L"skin", skin);
        }

        void Shape::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            type = loadParameter(componentData, nullptr, String());
            parameters = loadParameter(componentData, L"parameters", String());
            skin = loadParameter(componentData, L"skin", String());
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Shape)
        , public Plugin::ComponentMixin<Components::Shape>
    {
    public:
        Shape(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"shape";
        }
    };

    GEK_CONTEXT_USER(ShapeProcessor, Plugin::Core *)
        , public Plugin::PopulationListener
        , public Plugin::RendererListener
        , public Plugin::Processor
    {
    public:
        struct Shape
        {
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            uint32_t indexCount;
        };

        struct Data
        {
            Shape &shape;
            MaterialHandle skin;

            Data(Shape &shape, MaterialHandle skin)
                : shape(shape)
                , skin(skin)
            {
            }
        };

        __declspec(align(16))
            struct Instance
        {
            Math::Float4x4 matrix;

            Instance(const Math::Float4x4 &matrix)
                : matrix(matrix)
            {
            }
        };

    private:
        Plugin::Population *population;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        VisualHandle visual;
        Video::BufferPtr constantBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Shape> shapeMap;
        using EntityDataMap = concurrency::concurrent_unordered_map<Plugin::Entity *, Data>;
        EntityDataMap entityDataMap;

        using InstanceList = concurrency::concurrent_vector<Instance, AlignedAllocator<Instance, 16>>;
        using MaterialMap = concurrency::concurrent_unordered_map<MaterialHandle, InstanceList>;
        using VisibleMap = concurrency::concurrent_unordered_map<Shape *, MaterialMap>;
        VisibleMap visibleMap;

    public:
        ShapeProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
            , loadPool(1)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->addListener(this);
            renderer->addListener(this);

            visual = resources->loadVisual(L"shape");

            constantBuffer = renderer->getDevice()->createBuffer(sizeof(Instance), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
        }

        ~ShapeProcessor(void)
        {
            renderer->removeListener(this);
            population->removeListener(this);
        }

        // Plugin::PopulationListener
        void onLoadBegin(void)
        {
            loadPool.clear();
            shapeMap.clear();
            entityDataMap.clear();
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
        }

        void onEntityCreated(Plugin::Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<Components::Shape, Components::Transform>())
            {
                const auto &shapeComponent = entity->getComponent<Components::Shape>();
				auto hash = getHash(shapeComponent.parameters, shapeComponent.type);
                auto pair = shapeMap.insert(std::make_pair(hash, Shape()));
                if (pair.second)
                {
                    loadPool.enqueue([this, &shape = pair.first->second, type = String(shapeComponent.type), parameters = String(shapeComponent.parameters)](void) -> void
                    {
                        if (type.compareNoCase(L"sphere") == 0)
                        {
                            GeoSphere geoSphere;
                            uint32_t divisionCount = parameters;
                            geoSphere.generate(divisionCount);

                            shape.vertexBuffer = geoSphere.createVertexBuffer(String::create(L"shape:vertex:%v:%v", type, parameters), resources);
                            shape.indexBuffer = geoSphere.createIndexBuffer(String::create(L"shape:index:%v:%v", type, parameters), resources);
                            shape.indexCount = geoSphere.getIndexCount();
                        }
                        else if (type.compareNoCase(L"cube") == 0)
                        {
                            static const Vertex vertices[] =
                            {
                                { Math::Float3(-0.5f, +0.5f, -0.5f), Math::Float2(0.0f, 0.0f) }, //0
                                { Math::Float3(+0.5f, +0.5f, -0.5f), Math::Float2(1.0f, 0.0f) }, //1
                                { Math::Float3(+0.5f, +0.5f, +0.5f), Math::Float2(1.0f, 1.0f) }, //2
                                { Math::Float3(-0.5f, +0.5f, +0.5f), Math::Float2(0.0f, 1.0f) }, //3

                                { Math::Float3(-0.5f, -0.5f, -0.5f), Math::Float2(0.0f, 0.0f) }, //4
                                { Math::Float3(+0.5f, -0.5f, -0.5f), Math::Float2(1.0f, 0.0f) }, //5
                                { Math::Float3(+0.5f, -0.5f, +0.5f), Math::Float2(1.0f, 1.0f) }, //6
                                { Math::Float3(-0.5f, -0.5f, +0.5f), Math::Float2(0.0f, 1.0f) }, //7
                            };

                            static const uint16_t indices[] =
                            {
                                3,1,0,2,1,3, //top
                                0,5,4,1,5,0,
                                3,4,7,0,4,3,
                                1,6,5,2,6,1,
                                2,7,6,3,7,2,
                                6,4,5,7,4,6,
                            };

                            static const std::vector<uint8_t> vertexBuffer((uint8_t *)vertices, (uint8_t *)vertices + (sizeof(Vertex) * ARRAYSIZE(vertices)));
                            static const std::vector<uint8_t> indexBuffer((uint8_t *)indices, (uint8_t *)indices + (sizeof(uint16_t) * ARRAYSIZE(indices)));

                            shape.vertexBuffer = resources->createBuffer(String::create(L"shape:vertex:%v:%v", type, parameters), sizeof(Vertex), ARRAYSIZE(vertices), Video::BufferType::Vertex, 0, vertexBuffer);
                            shape.indexBuffer = resources->createBuffer(String::create(L"shape:index:%v:%v", type, parameters), Video::Format::R16_UINT, ARRAYSIZE(indices), Video::BufferType::Index, 0, indexBuffer);
                            shape.indexCount = 36;
                        }
                    });
                }

                Data data(pair.first->second, resources->loadMaterial(shapeComponent.skin));
                entityDataMap.insert(std::make_pair(entity, data));
            }
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entitySearch = entityDataMap.find(entity);
            if (entitySearch != entityDataMap.end())
            {
                entityDataMap.unsafe_erase(entitySearch);
            }
        }

        // Plugin::RendererListener
        static void drawCall(Video::Device::Context *deviceContext, Plugin::Resources *resources, const Shape *shape, const Instance *instanceList, Video::Buffer *constantBuffer)
        {
            Instance *instanceData = nullptr;
            deviceContext->getDevice()->mapBuffer(constantBuffer, (void **)&instanceData);
            memcpy(instanceData, instanceList, sizeof(Instance));
            deviceContext->getDevice()->unmapBuffer(constantBuffer);

            deviceContext->vertexPipeline()->setConstantBuffer(constantBuffer, 4);
            resources->setVertexBuffer(deviceContext, 0, shape->vertexBuffer, 0);
            resources->setIndexBuffer(deviceContext, shape->indexBuffer, 0);
            deviceContext->drawIndexedPrimitive(shape->indexCount, 0, 0);
        }

        void onRenderScene(const Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
        {
            GEK_REQUIRE(renderer);
            GEK_REQUIRE(cameraEntity);

            visibleMap.clear();
            concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entityDataPair) -> void
            {
                const Plugin::Entity *entity = entityDataPair.first;
                const Data &data = entityDataPair.second;

                const auto &transformComponent = entity->getComponent<Components::Transform>();
                Math::Float4x4 matrix(transformComponent.getMatrix());

                static const Shapes::AlignedBox unitCube(1.0f);
                Shapes::OrientedBox orientedBox(unitCube, matrix);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum.isVisible(orientedBox))
                {
                    auto &materialMap = visibleMap[&data.shape];
                    auto &instanceList = materialMap[data.skin];

                    auto instance(matrix * viewMatrix);
                    instanceList.push_back(Instance(instance));
                }
            });

            concurrency::parallel_for_each(visibleMap.begin(), visibleMap.end(), [&](auto &visibleMap) -> void
            {
                auto shape = visibleMap.first;
                if (shape->indexCount > 0)
                {
                    concurrency::parallel_for_each(visibleMap.second.begin(), visibleMap.second.end(), [&](auto &materialMap) -> void
                    {
                        concurrency::parallel_for_each(materialMap.second.begin(), materialMap.second.end(), [&](auto &instanceList) -> void
                        {
                            renderer->queueDrawCall(visual, materialMap.first, std::bind(drawCall, std::placeholders::_1, resources, shape, &instanceList, constantBuffer.get()));
                        });
                    });
                }
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Shape)
    GEK_REGISTER_CONTEXT_USER(ShapeProcessor)
}; // namespace Gek

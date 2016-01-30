﻿#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Color.h"
#include "GEK\Engine\Shape.h"
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <ppl.h>
#include <algorithm>
#include <memory>
#include <future>
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

        std::vector <Vertex> vertices;
        std::vector <Edge> edges;
        std::vector <Edge> newEdges;
        std::vector <Triangle> triangles;
        std::vector <Triangle> newTriangles;

        std::vector <UINT16> indices;
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
            Math::Float3 centroid = oneThird * (vertices[triangles[0].vertices[0]].position + vertices[triangles[0].vertices[1]].position + vertices[triangles[0].vertices[2]].position);
            return 1.0f / centroid.getLength();
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
                indices.push_back((UINT16)triangles[i].vertices[0]);
                indices.push_back((UINT16)triangles[i].vertices[1]);
                indices.push_back((UINT16)triangles[i].vertices[2]);
            }
        }

        const std::vector <Vertex> & getVertices() const
        {
            return vertices;
        }

        const std::vector <UINT16> & getIndices() const
        {
            return indices;
        }

        float getInscriptionRadiusMultiplier() const
        {
            return inscriptionRadiusMultiplier;
        }
    };

    static const UINT32 MaxInstanceCount = 500;

    class ShapeProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        struct Shape
        {
            std::atomic<bool> loaded;

            CStringW parameters;
            Shapes::AlignedBox alignedBox;
            MaterialHandle material;
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            UINT32 indexCount;

            Shape(void)
                : loaded(false)
                , indexCount(0)
            {
            }
        };

        struct InstanceData
        {
            Math::Float4x4 matrix;
            Math::Float4 color;
            Math::Float4 scale;

            InstanceData(const Math::Float4x4 &matrix, const Math::Float4 &color, const Math::Float3 &scale)
                : matrix(matrix)
                , color(color)
                , scale(scale)
            {
            }
        };

    private:
        PluginResources *resources;
        Render *render;
        Population *population;

        PluginHandle plugin;
        ResourceHandle constantBuffer;

        std::future<void> loadShapeRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadShapeQueue;
        concurrency::concurrent_unordered_map<CStringW, bool> loadShapeSet;

        std::unordered_map<CStringW, Shape> dataMap;
        std::unordered_map<Entity *, Shape *> dataEntityList;
        concurrency::concurrent_unordered_map<Shape *, concurrency::concurrent_vector<InstanceData>> visibleList;

    public:
        ShapeProcessorImplementation(void)
            : resources(nullptr)
            , render(nullptr)
            , population(nullptr)
        {
        }

        ~ShapeProcessorImplementation(void)
        {
            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(ShapeProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        HRESULT loadBoundingBox(Shape *data, const CStringW &parameters)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);

            gekCheckScope(resultValue, parameters);

            data->parameters = parameters;

            int position = 0;
            CStringW shape = data->parameters.Tokenize(L"|", position);
            if (shape.CompareNoCase(L"cube") == 0)
            {
                resultValue = E_FAIL;
            }
            else if (shape.CompareNoCase(L"sphere") == 0)
            {
                resultValue = S_OK;
            }
            else
            {
                resultValue = E_FAIL;
            }

            if (SUCCEEDED(resultValue))
            {
                data->alignedBox.minimum = Math::Float3(-1.0f);
                data->alignedBox.maximum = Math::Float3(1.0f);
            }

            return resultValue;
        }

        HRESULT loadShapeWorker(Shape *data)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);

            gekCheckScope(resultValue, data->parameters.GetString());

            int position = 0;
            CStringW shape = data->parameters.Tokenize(L"|", position);
            CStringW material = data->parameters.Tokenize(L"|", position);
            if (shape.CompareNoCase(L"cube") == 0)
            {
                resultValue = E_FAIL;
            }
            else if (shape.CompareNoCase(L"sphere") == 0)
            {
                UINT32 divisionCount = String::to<UINT32>(data->parameters.Tokenize(L"|", position));

                GeoSphere geoSphere;
                geoSphere.generate(divisionCount);

                data->material = resources->loadMaterial(material);
                data->indexCount = geoSphere.getIndices().size();
                data->vertexBuffer = resources->createBuffer(String::format(L"shape:vertex:%s:%d", shape.GetString(), divisionCount), sizeof(Vertex), geoSphere.getVertices().size(), Video::BufferType::Vertex, 0, geoSphere.getVertices().data());
                data->indexBuffer = resources->createBuffer(String::format(L"shape:index:%s:%d", shape.GetString(), divisionCount), Video::Format::Short, geoSphere.getIndices().size(), Video::BufferType::Index, 0, geoSphere.getIndices().data());
                resultValue = ((data->vertexBuffer.isValid() && data->indexBuffer.isValid()) ? S_OK : E_FAIL);
            }
            else
            {
                resultValue = E_FAIL;
            }

            if (SUCCEEDED(resultValue))
            {
                data->loaded = true;
            }

            return resultValue;
        }

        HRESULT loadShape(Shape *data)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);

            if (loadShapeSet.count(data->parameters) > 0)
            {
                return S_OK;
            }

            loadShapeSet.insert(std::make_pair(data->parameters, true));
            loadShapeQueue.push(std::bind(&ShapeProcessorImplementation::loadShapeWorker, this, data));
            if (!loadShapeRunning.valid() || (loadShapeRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
            {
                loadShapeRunning = std::async(std::launch::async, [&](void) -> void
                {
                    CoInitialize(nullptr);
                    std::function<void(void)> function;
                    while (loadShapeQueue.try_pop(function))
                    {
                        function();
                    };

                    CoUninitialize();
                });
            }

            return S_OK;
        }

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            gekCheckScope(resultValue);

            CComQIPtr<PluginResources> resources(initializerContext);
            CComQIPtr<Render> render(initializerContext);
            CComQIPtr<Population> population(initializerContext);
            if (resources && render && population)
            {
                this->resources = resources;
                this->render = render;
                this->population = population;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = ObservableMixin::addObserver(render, getClass<RenderObserver>());
            }

            if (SUCCEEDED(resultValue))
            {
                plugin = resources->loadPlugin(L"shape");
                if (!plugin.isValid())
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                constantBuffer = resources->createBuffer(nullptr, sizeof(InstanceData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                if (!constantBuffer.isValid())
                {
                    resultValue = E_FAIL;
                }
            }

            return resultValue;
        };

        // PopulationObserver
        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            loadShapeSet.clear();
            loadShapeQueue.clear();
            if (loadShapeRunning.valid() && (loadShapeRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
            {
                loadShapeRunning.get();
            }

            dataMap.clear();
            dataEntityList.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(population);

            if (entity->hasComponents<ShapeComponent, TransformComponent>())
            {
                auto &shapeComponent = entity->getComponent<ShapeComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();
                auto dataNameIterator = dataMap.find(shapeComponent);
                if (dataNameIterator != dataMap.end())
                {
                    dataEntityList[entity] = &(*dataNameIterator).second;
                }
                else
                {
                    Shape &data = dataMap[shapeComponent];
                    loadBoundingBox(&data, shapeComponent);
                    dataEntityList[entity] = &data;
                }
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            auto dataEntityIterator = dataEntityList.find(entity);
            if (dataEntityIterator != dataEntityList.end())
            {
                dataEntityList.erase(dataEntityIterator);
            }
        }

        // RenderObserver
        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Gek::Shapes::Frustum *viewFrustum)
        {
            REQUIRE_VOID_RETURN(population);
            REQUIRE_VOID_RETURN(viewFrustum);

            visibleList.clear();
            std::for_each(dataEntityList.begin(), dataEntityList.end(), [&](const std::pair<Entity *, Shape *> &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                Shape &data = *dataEntity.second;

                const auto &shapeComponent = entity->getComponent<ShapeComponent>();
                const auto &transformComponent = entity->getComponent<TransformComponent>();
                Shapes::OrientedBox orientedBox(data.alignedBox, transformComponent.rotation, transformComponent.position);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum->isVisible(orientedBox))
                {
                    Gek::Math::Float4 color(1.0f, 1.0f, 1.0f, 1.0f);
                    if (entity->hasComponent<ColorComponent>())
                    {
                        color = entity->getComponent<ColorComponent>();
                    }

                    visibleList[&data].push_back(InstanceData(transformComponent.getMatrix(), color, transformComponent.scale));
                }
            });

            for (auto &visible : visibleList)
            {
                Shape &data = *(visible.first);
                if (data.loaded)
                {
                    for (auto &instance : visible.second)
                    {
                        static auto drawCall = [](VideoContext *videoContext, PluginResources *resources, Shape *shape, InstanceData *instance, ResourceHandle constantBuffer) -> void
                        {
                            LPVOID instanceData;
                            if (SUCCEEDED(resources->mapBuffer(constantBuffer, &instanceData)))
                            {
                                memcpy(instanceData, instance, sizeof(InstanceData));
                                resources->unmapBuffer(constantBuffer);

                                resources->setConstantBuffer(videoContext->vertexPipeline(), constantBuffer, 2);
                                resources->setVertexBuffer(videoContext, 0, shape->vertexBuffer, 0);
                                resources->setIndexBuffer(videoContext, shape->indexBuffer, 0);
                                videoContext->drawIndexedPrimitive(shape->indexCount, 0, 0);
                            }
                        };

                        render->queueDrawCall(plugin, data.material, std::bind(drawCall, std::placeholders::_1, resources, &data, &instance, constantBuffer));
                    }
                }
                else
                {
                    loadShape(&data);
                }
            }
        }
    };

    REGISTER_CLASS(ShapeProcessorImplementation)
}; // namespace Gek
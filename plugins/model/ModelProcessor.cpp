#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Shape\OrientedBox.h"
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
#include "GEK\Engine\Model.h"
#include <memory>
#include <map>
#include <set>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <ppl.h>
#include <algorithm>
#include <array>

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

    class ModelProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        struct MaterialInfo
        {
            MaterialHandle material;
            UINT32 firstVertex;
            UINT32 firstIndex;
            UINT32 indexCount;

            MaterialInfo(void)
                : firstVertex(0)
                , firstIndex(0)
                , indexCount(0)
            {
            }
        };

        struct ModelData
        {
            bool loaded;
            Shape::AlignedBox alignedBox;
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            std::vector<MaterialInfo> materialInfoList;

            ModelData(void)
                : loaded(false)
            {
            }
        };

        struct InstanceData
        {
            Math::Float4x4 matrix;
            Math::Float4 color;
            Math::Float3 scale;
            float distance;

            InstanceData(const Math::Float4x4 &matrix, const Math::Float4 &color, const Math::Float3 &scale, float distance)
                : matrix(matrix)
                , color(color)
                , scale(scale)
                , distance(distance)
            {
            }
        };

    private:
        PluginResources *resources;
        Render *render;
        Population *population;

        PluginHandle plugin;

        std::unordered_map<ModelData *, std::function<HRESULT(void)>> dataLoadQueue;
        std::unordered_map<CStringW, ModelData> dataMap;
        std::unordered_map<Entity *, ModelData *> dataEntityList;
        ResourceHandle instanceBuffer;

        std::list<std::unique_ptr<std::thread>> loadList;

    public:
        ModelProcessorImplementation(void)
            : resources(nullptr)
            , render(nullptr)
            , population(nullptr)
        {
        }

        ~ModelProcessorImplementation(void)
        {
            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(ModelProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        HRESULT loadShape(ModelData *data, CStringW parameters)
        {
            gekCheckScope(resultValue, parameters);

            int position = 0;
            CStringW shape = parameters.Tokenize(L"|", position);
            CStringW materialName = parameters.Tokenize(L"|", position);
            MaterialHandle material = resources->loadMaterial(materialName);
            if (material.isValid())
            {
                resultValue = S_OK;
                if (shape.CompareNoCase(L"cube") == 0)
                {
                    resultValue = E_FAIL;
                }
                else if (shape.CompareNoCase(L"sphere") == 0)
                {
                    UINT32 divisionCount = String::to<UINT32>(parameters.Tokenize(L"|", position));

                    GeoSphere geoSphere;
                    geoSphere.generate(divisionCount);

                    MaterialInfo materialInfo;
                    materialInfo.firstVertex = 0;
                    materialInfo.firstIndex = 0;
                    materialInfo.indexCount = geoSphere.getIndices().size();
                    materialInfo.material = material;
                    data->materialInfoList.push_back(materialInfo);

                    data->vertexBuffer = resources->createBuffer(String::format(L"model:vertex:%s:%d", shape.GetString(), divisionCount), sizeof(Vertex), geoSphere.getVertices().size(), Video::BufferType::Vertex, 0, geoSphere.getVertices().data());
                    data->indexBuffer = resources->createBuffer(String::format(L"model:index:%s:%d", shape.GetString(), divisionCount), Video::Format::Short, geoSphere.getIndices().size(), Video::BufferType::Index, 0, geoSphere.getIndices().data());
                    resultValue = ((data->vertexBuffer.isValid() && data->indexBuffer.isValid()) ? S_OK : E_FAIL);
                }
                else
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                data->loaded = true;
            }

            return resultValue;
        }

        HRESULT preLoadShape(ModelData *data, CStringW parameters)
        {
            gekCheckScope(resultValue, parameters);

            int position = 0;
            CStringW shape = parameters.Tokenize(L"|", position);
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
                auto loadIterator = dataLoadQueue.find(data);
                if (loadIterator == dataLoadQueue.end())
                {
                    dataLoadQueue[data] = std::bind(&ModelProcessorImplementation::loadShape, this, data, CStringW(parameters));
                }
            }

            return resultValue;
        }

        HRESULT loadModel(ModelData *data, CStringW fileName, CStringW name)
        {
            gekCheckScope(resultValue, fileName, name);

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(fileName, fileData);
            if (SUCCEEDED(resultValue))
            {
                UINT8 *rawFileData = fileData.data();
                UINT32 gekIdentifier = *((UINT32 *)rawFileData);
                rawFileData += sizeof(UINT32);

                UINT16 gekModelType = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                UINT16 gekModelVersion = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                resultValue = E_INVALIDARG;
                if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 2)
                {
                    data->alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                    rawFileData += sizeof(Gek::Shape::AlignedBox);

                    UINT32 materialCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    resultValue = S_OK;
                    data->materialInfoList.resize(materialCount);
                    for (UINT32 materialIndex = 0; materialIndex < materialCount; ++materialIndex)
                    {
                        CStringA materialNameUtf8(rawFileData);
                        rawFileData += (materialNameUtf8.GetLength() + 1);

                        MaterialInfo &materialInfo = data->materialInfoList[materialIndex];
                        materialInfo.material = resources->loadMaterial(CA2W(materialNameUtf8, CP_UTF8));
                        if (!materialInfo.material.isValid())
                        {
                            resultValue = E_FAIL;
                            break;
                        }

                        materialInfo.firstVertex = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        materialInfo.firstIndex = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        materialInfo.indexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        UINT32 vertexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        data->vertexBuffer = resources->createBuffer(String::format(L"model:vertex:%s", name.GetString()), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, rawFileData);
                        rawFileData += (sizeof(Vertex) * vertexCount);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        UINT32 indexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        data->indexBuffer = resources->createBuffer(String::format(L"model:index:%s", name.GetString()), Video::Format::Short, indexCount, Video::BufferType::Index, 0, rawFileData);
                        rawFileData += (sizeof(UINT16) * indexCount);
                    }
                }
                else
                {
                    gekLogMessage(L"Invalid GEK model data found: ID(%d) Type(%d) Version(%d)", gekIdentifier, gekModelType, gekModelVersion);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                data->loaded = true;
            }

            return resultValue;
        }

        HRESULT preLoadModel(ModelData *data, LPCWSTR fileName, LPCWSTR name)
        {
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, name);

            static const UINT32 PreReadSize = (sizeof(UINT32) + sizeof(UINT16) + sizeof(UINT16) + sizeof(Shape::AlignedBox));

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(fileName, fileData, PreReadSize);
            if (SUCCEEDED(resultValue))
            {
                UINT8 *rawFileData = fileData.data();
                UINT32 gekIdentifier = *((UINT32 *)rawFileData);
                rawFileData += sizeof(UINT32);

                UINT16 gekModelType = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                UINT16 gekModelVersion = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                resultValue = E_INVALIDARG;
                if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 2)
                {
                    data->alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                    resultValue = S_OK;
                }
                else
                {
                    gekLogMessage(L"Invalid GEK model data found: ID(%d) Type(%d) Version(%d)", gekIdentifier, gekModelType, gekModelVersion);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                auto loadIterator = dataLoadQueue.find(data);
                if (loadIterator == dataLoadQueue.end())
                {
                    dataLoadQueue[data] = std::bind(&ModelProcessorImplementation::loadModel, this, data, CStringW(fileName), CStringW(name));
                }
            }

            return resultValue;
        }

        HRESULT preLoadData(ModelData *data, LPCWSTR model)
        {
            REQUIRE_RETURN(model, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            if (*model == L'*')
            {
                resultValue = preLoadShape(data, &model[1]);
            }
            else
            {
                CStringW fileName;
                fileName.Format(L"%%root%%\\data\\models\\%s.gek", model);
                resultValue = preLoadModel(data, fileName, model);
            }

            return resultValue;
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
                plugin = resources->loadPlugin(L"model");
                if (!plugin.isValid())
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                instanceBuffer = resources->createBuffer(nullptr, sizeof(InstanceData), 1024, Video::BufferType::Vertex, Video::BufferFlags::Mappable);
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
            dataMap.clear();
            dataEntityList.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(population);

            if (entity->hasComponents<ModelComponent, TransformComponent>())
            {
                auto &modelComponent = entity->getComponent<ModelComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();
                auto dataNameIterator = dataMap.find(modelComponent);
                if (dataNameIterator != dataMap.end())
                {
                    dataEntityList[entity] = &(*dataNameIterator).second;
                }
                else
                {
                    ModelData &data = dataMap[modelComponent];
                    preLoadData(&data, modelComponent);
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
        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Gek::Shape::Frustum *viewFrustum)
        {
            REQUIRE_VOID_RETURN(population);
            REQUIRE_VOID_RETURN(viewFrustum);

            const auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();

            concurrency::concurrent_unordered_map<ModelData *, concurrency::concurrent_vector<InstanceData>> visibleList;
            concurrency::parallel_for_each(dataEntityList.begin(), dataEntityList.end(), [&](const std::pair<Entity *, ModelData *> &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                ModelData *data = dataEntity.second;

                const auto &transformComponent = entity->getComponent<TransformComponent>();
                Shape::OrientedBox orientedBox(data->alignedBox, transformComponent.rotation, transformComponent.position);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum->isVisible(orientedBox))
                {
                    Gek::Math::Float4 color(1.0f, 1.0f, 1.0f, 1.0f);
                    if (entity->hasComponent<ColorComponent>())
                    {
                        color = entity->getComponent<ColorComponent>();
                    }

                    visibleList[data].push_back(InstanceData(transformComponent.getMatrix(), color, transformComponent.scale, cameraTransform.position.getDistance(transformComponent.position)));
                }
            });

            std::vector<InstanceData> instanceArray;
            std::map<ModelData *, std::pair<UINT32, UINT32>> instanceMap;
            for (auto instancePair : visibleList)
            {
                ModelData *data = (instancePair.first);
                auto loadIterator = dataLoadQueue.find(data);
                if (loadIterator != dataLoadQueue.end())
                {
                    auto load = (*loadIterator).second;
                    dataLoadQueue.erase(loadIterator);
                    load();
                }

                if (data->loaded)
                {
                    auto &instanceList = instancePair.second;
                    concurrency::parallel_sort(instanceList.begin(), instanceList.end(), [&](const InstanceData &leftInstance, const InstanceData &rightInstance) -> bool
                    {
                        return (leftInstance.distance < rightInstance.distance);
                    });

                    instanceMap[data] = std::make_pair(instanceList.size(), instanceArray.size());
                    instanceArray.insert(instanceArray.end(), instanceList.begin(), instanceList.end());
                }
            }

            LPVOID instanceData = nullptr;
            if (SUCCEEDED(resources->mapBuffer(instanceBuffer, &instanceData)))
            {
                UINT32 instanceCount = std::min(instanceArray.size(), size_t(1024));
                memcpy(instanceData, instanceArray.data(), (sizeof(InstanceData) * instanceCount));
                resources->unmapBuffer(instanceBuffer);

                for (auto &instancePair : instanceMap)
                {
                    auto data = instancePair.first;
                    for (auto &materialInfo : data->materialInfoList)
                    {
                        static auto drawCall = [](VideoContext *videoContext, PluginResources *resources, ResourceHandle vertexBuffer, ResourceHandle instanceBuffer, ResourceHandle indexBuffer, UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) -> void
                        {
                            resources->setVertexBuffer(videoContext, 0, vertexBuffer, 0);
                            resources->setVertexBuffer(videoContext, 1, instanceBuffer, 0);
                            resources->setIndexBuffer(videoContext, indexBuffer, 0);
                            videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
                        };

                        render->queueDrawCall(plugin, materialInfo.material, std::bind(drawCall, std::placeholders::_1, resources, data->vertexBuffer, instanceBuffer, data->indexBuffer, instancePair.second.first, instancePair.second.second, materialInfo.indexCount, materialInfo.firstIndex, materialInfo.firstVertex));
                    }
                }
            }
        }
    };

    REGISTER_CLASS(ModelProcessorImplementation)
}; // namespace Gek

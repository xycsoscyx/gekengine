#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Shape\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Size.h"
#include "GEK\Components\Color.h"
#include "GEK\Engine\Model.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <memory>
#include <map>
#include <set>
#include <ppl.h>
#include <algorithm>

#undef min

namespace Gek
{
    namespace Model
    {
        static const UINT32 MaxInstanceCount = 500;

        class System : public Context::BaseUser
            , public BaseObservable
            , public Engine::Population::Observer
            , public Engine::Render::Observer
            , public Engine::System::Interface
        {
        public:
            struct Vertex
            {
                Math::Float3 position;
                Math::Float2 texCoord;
                Math::Float3 normal;
            };

            struct MaterialInfo
            {
                CComPtr<IUnknown> material;
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

            struct Data
            {
                bool loaded;
                bool ready;
                CStringW fileName;
                Shape::AlignedBox alignedBox;
                CComPtr<Video3D::BufferInterface> vertexBuffer;
                CComPtr<Video3D::BufferInterface> indexBuffer;
                std::vector<MaterialInfo> materialInfoList;

                Data(void)
                    : loaded(false)
                    , ready(false)
                {
                }
            };

            struct Instance
            {
                Math::Float4x4 matrix;
                Math::Float3 size;
                Math::Float4 color;
                float distance;

                Instance(const Math::Float4x4 &matrix, const Math::Float3 &size, const Math::Float4 &color, float distance)
                    : matrix(matrix)
                    , size(size)
                    , color(color)
                    , distance(distance)
                {
                }
            };

        private:
            Video3D::Interface *video;
            Engine::Render::Interface *render;
            Engine::Population::Interface *population;

            CComPtr<IUnknown> plugin;

            concurrency::concurrent_unordered_map<CStringW, Data> dataMap;
            concurrency::concurrent_unordered_map<Engine::Population::Entity, Data *> dataEntityList;
            concurrency::concurrent_unordered_map<Data *, std::vector<Instance>> visibleList;

        public:
            System(void)
                : render(nullptr)
                , video(nullptr)
                , population(nullptr)
            {
            }

            ~System(void)
            {
                BaseObservable::removeObserver(render, getClass<Engine::Render::Observer>());
                BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::Render::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::System::Interface)
            END_INTERFACE_LIST_USER

            HRESULT loadDataSize(LPCWSTR fileName, Data &data)
            {
                gekLogScope(__FUNCTION__);

                static const UINT32 nPreReadSize = (sizeof(UINT32) + sizeof(UINT16) + sizeof(UINT16) + sizeof(Shape::AlignedBox));

                std::vector<UINT8> fileData;
                data.fileName.Format(L"%%root%%\\data\\models\\%s.gek", fileName);
                HRESULT resultValue = Gek::FileSystem::load(data.fileName, fileData, nPreReadSize);
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
                        data.alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                        resultValue = S_OK;
                    }
                }

                return resultValue;
            }

            HRESULT loadData(Data &data)
            {
                if (data.loaded)
                {
                    return S_OK;
                }

                gekLogScope(__FUNCTION__);

                data.loaded = true;
                std::vector<UINT8> fileData;
                HRESULT resultValue = Gek::FileSystem::load(data.fileName, fileData);
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
                        data.alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                        rawFileData += sizeof(Gek::Shape::AlignedBox);

                        UINT32 materialCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        resultValue = S_OK;
                        data.materialInfoList.resize(materialCount);
                        for (UINT32 materialIndex = 0; materialIndex < materialCount; ++materialIndex)
                        {
                            CStringA materialNameUtf8(rawFileData);
                            rawFileData += (materialNameUtf8.GetLength() + 1);

                            MaterialInfo &materialInfo = data.materialInfoList[materialIndex];
                            resultValue = render->loadMaterial(&materialInfo.material, CA2W(materialNameUtf8, CP_UTF8));
                            if (!materialInfo.material)
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

                            resultValue = video->createBuffer(&data.vertexBuffer, sizeof(Vertex), vertexCount, Video3D::BufferFlags::VertexBuffer | Video3D::BufferFlags::Static, rawFileData);
                            rawFileData += (sizeof(Vertex) * vertexCount);
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            UINT32 indexCount = *((UINT32 *)rawFileData);
                            rawFileData += sizeof(UINT32);

                            resultValue = video->createBuffer(&data.indexBuffer, sizeof(UINT16), indexCount, Video3D::BufferFlags::IndexBuffer | Video3D::BufferFlags::Static, rawFileData);
                            rawFileData += (sizeof(UINT16) * indexCount);
                        }
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    data.ready = true;
                }

                return resultValue;
            }

            // System::Interface
            STDMETHODIMP initialize(IUnknown *initializerContext)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Video3D::Interface> video(initializerContext);
                CComQIPtr<Engine::Render::Interface> render(initializerContext);
                CComQIPtr<Engine::Population::Interface> population(initializerContext);
                if (render && video && population)
                {
                    this->video = video;
                    this->render = render;
                    this->population = population;
                    resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = BaseObservable::addObserver(render, getClass<Engine::Render::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = render->loadPlugin(&plugin, L"model");
                }

                return resultValue;
            };

            // Population::Observer
            STDMETHODIMP_(void) onLoadBegin(void)
            {
            }

            STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
            {
                if (FAILED(resultValue))
                {
                    onFree();
                }
            }

            STDMETHODIMP_(void) onFree(void)
            {
                REQUIRE_VOID_RETURN(video);

                dataMap.clear();
                dataEntityList.clear();
            }

            STDMETHODIMP_(void) onEntityCreated(const Engine::Population::Entity &entity)
            {
                REQUIRE_VOID_RETURN(population);

                if (population->hasComponent(entity, Model::identifier) &&
                    population->hasComponent(entity, Engine::Components::Transform::identifier))
                {
                    auto &modelComponent = population->getComponent<Model::Data>(entity, Model::identifier);
                    auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entity, Engine::Components::Transform::identifier);
                    auto dataNameIterator = dataMap.find(modelComponent);
                    if (dataNameIterator != dataMap.end())
                    {
                        dataEntityList[entity] = &(*dataNameIterator).second;
                    }
                    else
                    {
                        Data &data = dataMap[modelComponent];
                        loadDataSize(modelComponent, data);
                        dataEntityList[entity] = &data;
                    }
                }
            }

            STDMETHODIMP_(void) onEntityDestroyed(const Engine::Population::Entity &entity)
            {
                auto dataEntityIterator = dataEntityList.find(entity);
                if (dataEntityIterator != dataEntityList.end())
                {
                    dataEntityList.unsafe_erase(dataEntityIterator);
                }
            }

            // Render::Observer
            STDMETHODIMP_(void) OnRenderScene(const Engine::Population::Entity &cameraEntity, const Gek::Shape::Frustum &viewFrustum)
            {
                REQUIRE_VOID_RETURN(population);

                auto &cameraTransform = population->getComponent<Engine::Components::Transform::Data>(cameraEntity, Engine::Components::Transform::identifier);

                visibleList.clear();
                for (auto dataEntity : dataEntityList)
                {
                    Data &data = *(dataEntity.second);
                    Gek::Math::Float3 size(1.0f, 1.0f, 1.0f);
                    if (population->hasComponent(dataEntity.first, Engine::Components::Size::identifier))
                    {
                        size = population->getComponent<Engine::Components::Size::Data>(dataEntity.first, Engine::Components::Size::identifier);
                    }

                    Gek::Shape::AlignedBox alignedBox(data.alignedBox);
                    alignedBox.minimum *= size;
                    alignedBox.maximum *= size;

                    auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(dataEntity.first, Engine::Components::Transform::identifier);
                    Shape::OrientedBox orientedBox(alignedBox, transformComponent.rotation, transformComponent.position);
                    if (viewFrustum.isVisible(orientedBox))
                    {
                        Gek::Math::Float4 color(1.0f, 1.0f, 1.0f, 1.0f);
                        if (population->hasComponent(dataEntity.first, Engine::Components::Color::identifier))
                        {
                            color = population->getComponent<Engine::Components::Color::Data>(dataEntity.first, Engine::Components::Color::identifier);
                        }

                        visibleList[dataEntity.second].push_back(Instance(orientedBox.matrix, size, color, cameraTransform.position.getDistance(orientedBox.matrix.translation)));
                    }
                }

                for (auto instancePair : visibleList)
                {
                    Data &data = *(instancePair.first);
                    if (SUCCEEDED(loadData(data)) && data.ready)
                    {
                        auto &instanceList = instancePair.second;
                        concurrency::parallel_sort(instanceList.begin(), instanceList.end(), [](const Instance &leftInstance, const Instance &rightInstance) -> bool
                        {
                            return (leftInstance.distance < rightInstance.distance);
                        });

                        for (auto &materialInfo : data.materialInfoList)
                        {
                            render->drawInstancedIndexedPrimitive(plugin, materialInfo.material, instanceList.data(), sizeof(Instance), instanceList.size(), data.vertexBuffer, materialInfo.firstVertex, data.indexBuffer, materialInfo.indexCount, materialInfo.firstIndex);
                        }
                    }
                }
            }

            STDMETHODIMP_(void) onRenderOverlay(void)
            {
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Model
}; // namespace Gek

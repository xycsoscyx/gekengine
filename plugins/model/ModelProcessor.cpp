#include "GEK\Math\Matrix4x4.h"
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
#include "GEK\Engine\Model.h"
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
    class ModelProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
            Math::Float3 normal;
        };

        struct SubModel
        {
            bool skin;
            MaterialHandle material;
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            UINT32 indexCount;
        };

        struct Model
        {
            std::atomic<bool> loaded;

            CStringW name;
            CStringW fileName;
            Shapes::AlignedBox alignedBox;
            std::vector<SubModel> subModelList;

            Model(void)
                : loaded(false)
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

        std::future<void> loadModelRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadModelQueue;
        concurrency::concurrent_unordered_map<CStringW, bool> loadModelSet;

        std::unordered_map<CStringW, Model> dataMap;
        std::unordered_map<Entity *, Model *> dataEntityList;
        concurrency::concurrent_unordered_map<Model *, concurrency::concurrent_vector<InstanceData>> visibleList;

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

        HRESULT loadBoundingBox(Model *data, LPCWSTR name)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);
            REQUIRE_RETURN(name, E_INVALIDARG);

            gekCheckScope(resultValue, name);

            data->name = name;
            data->fileName.Format(L"%%root%%\\data\\models\\%s.gek", data->name.GetString());
            static const UINT32 PreReadSize = (sizeof(UINT32) + sizeof(UINT16) + sizeof(UINT16) + sizeof(Shapes::AlignedBox));

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(data->fileName, fileData, PreReadSize);
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
                if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 3)
                {
                    data->alignedBox = *(Gek::Shapes::AlignedBox *)rawFileData;
                    resultValue = S_OK;
                }
                else
                {
                    gekLogMessage(L"Invalid GEK model data found: ID(%d) Type(%d) Version(%d)", gekIdentifier, gekModelType, gekModelVersion);
                }
            }

            return resultValue;
        }

        HRESULT loadModelWorker(Model *data)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);

            gekCheckScope(resultValue, data->name.GetString());

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(data->fileName, fileData);
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
                if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 3)
                {
                    data->alignedBox = *(Gek::Shapes::AlignedBox *)rawFileData;
                    rawFileData += sizeof(Gek::Shapes::AlignedBox);

                    UINT32 subModelCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    resultValue = S_OK;
                    data->subModelList.resize(subModelCount);
                    for (UINT32 modelIndex = 0; modelIndex < subModelCount; ++modelIndex)
                    {
                        CStringW materialName = LPCWSTR(rawFileData);
                        rawFileData += ((materialName.GetLength() + 1) * sizeof(wchar_t));

                        SubModel &subModel = data->subModelList[modelIndex];
                        if (materialName.CompareNoCase(L"skin") == 0)
                        {
                            subModel.skin = true;
                        }
                        else
                        {
                            subModel.material = resources->loadMaterial(materialName);
                            if (!subModel.material.isValid())
                            {
                                resultValue = E_FAIL;
                                break;
                            }
                        }

                        UINT32 vertexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        subModel.vertexBuffer = resources->createBuffer(String::format(L"model:vertex:%d:%s", modelIndex, data->name.GetString()), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, rawFileData);
                        rawFileData += (sizeof(Vertex) * vertexCount);

                        UINT32 indexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        subModel.indexCount = indexCount;
                        subModel.indexBuffer = resources->createBuffer(String::format(L"model:index:%d:%s", modelIndex, data->name.GetString()), Video::Format::Short, indexCount, Video::BufferType::Index, 0, rawFileData);
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

        HRESULT loadModel(Model *data)
        {
            REQUIRE_RETURN(data, E_INVALIDARG);

            if (loadModelSet.count(data->name) > 0)
            {
                return S_OK;
            }

            loadModelSet.insert(std::make_pair(data->name, true));
            loadModelQueue.push(std::bind(&ModelProcessorImplementation::loadModelWorker, this, data));
            if (!loadModelRunning.valid() || (loadModelRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
            {
                loadModelRunning = std::async(std::launch::async, [&](void) -> void
                {
                    CoInitialize(nullptr);
                    std::function<void(void)> function;
                    while (loadModelQueue.try_pop(function))
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
                plugin = resources->loadPlugin(L"model");
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
            loadModelSet.clear();
            loadModelQueue.clear();
            if (loadModelRunning.valid() && (loadModelRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
            {
                loadModelRunning.get();
            }

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
                    Model &data = dataMap[modelComponent];
                    loadBoundingBox(&data, modelComponent);
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
            std::for_each(dataEntityList.begin(), dataEntityList.end(), [&](const std::pair<Entity *, Model *> &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                Model &data = *dataEntity.second;

                const auto &modelComponent = entity->getComponent<ModelComponent>();
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
                Model &data = *(visible.first);
                if (data.loaded)
                {
                    for (auto &instance : visible.second)
                    {
                        static auto drawCall = [](VideoContext *videoContext, PluginResources *resources, SubModel *subModel, InstanceData *instance, ResourceHandle constantBuffer) -> void
                        {
                            LPVOID instanceData;
                            if (SUCCEEDED(resources->mapBuffer(constantBuffer, &instanceData)))
                            {
                                memcpy(instanceData, instance, sizeof(InstanceData));
                                resources->unmapBuffer(constantBuffer);

                                resources->setConstantBuffer(videoContext->vertexPipeline(), constantBuffer, 2);
                                resources->setVertexBuffer(videoContext, 0, subModel->vertexBuffer, 0);
                                resources->setIndexBuffer(videoContext, subModel->indexBuffer, 0);
                                videoContext->drawIndexedPrimitive(subModel->indexCount, 0, 0);
                            }
                        };

                        for (auto &subModel : data.subModelList)
                        {
                            render->queueDrawCall(plugin, subModel.material, std::bind(drawCall, std::placeholders::_1, resources, &subModel, &instance, constantBuffer));
                        }
                    }
                }
                else
                {
                    loadModel(&data);
                }
            }
        }
    };

    REGISTER_CLASS(ModelProcessorImplementation)
}; // namespace Gek

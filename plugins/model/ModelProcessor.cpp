﻿#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\ContextUser.h"
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
    class ModelProcessorImplementation
        : public ContextUserMixin
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

            SubModel(void)
                : skin(false)
                , indexCount(0)
            {
            }

            SubModel(const SubModel &subModel)
                : skin(subModel.skin)
                , material(subModel.material)
                , vertexBuffer(subModel.vertexBuffer)
                , indexBuffer(subModel.indexBuffer)
                , indexCount(subModel.indexCount)
            {
            }
        };

        struct Model
        {
            std::atomic<bool> loaded;
            wstring fileName;
            Shapes::AlignedBox alignedBox;
            std::vector<SubModel> subModelList;

            Model(void)
                : loaded(false)
            {
            }

            Model(const Model &model)
                : loaded(model.loaded ? true : false)
                , fileName(model.fileName)
                , alignedBox(model.alignedBox)
                , subModelList(model.subModelList)
            {
            }
        };

        struct EntityData
        {
            Model &model;
            MaterialHandle skin;

            EntityData(Model &model, MaterialHandle skin)
                : model(model)
                , skin(skin)
            {
            }
        };

        __declspec(align(16))
        struct InstanceData
        {
            Math::Float4x4 matrix;
            Math::Color color;
            Math::Float3 scale;
            float buffer;

            InstanceData(const Math::Float4x4 &matrix, const Math::Color &color, const Math::Float3 &scale)
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
        concurrency::concurrent_unordered_map<std::size_t, bool> loadModelSet;

        std::unordered_map<std::size_t, Model> dataMap;
        typedef std::unordered_map<Entity *, EntityData> DataEntityMap;
        DataEntityMap entityDataList;

        typedef concurrency::concurrent_vector<InstanceData, AlignedAllocator<InstanceData, 16>> InstanceList;
        typedef concurrency::concurrent_unordered_map<MaterialHandle, InstanceList> MaterialList;
        typedef concurrency::concurrent_unordered_map<Model *, MaterialList> VisibleList;
        VisibleList visibleList;

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

        HRESULT loadBoundingBox(Model &model, const wstring &name)
        {
            static const UINT32 PreReadSize = (sizeof(UINT32) + sizeof(UINT16) + sizeof(UINT16) + sizeof(Shapes::AlignedBox));

            HRESULT resultValue = E_FAIL;

            model.fileName.Format(L"$root\\data\\models\\%.gek", name.GetString());

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(model.fileName, fileData, PreReadSize);
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
                    model.alignedBox = *(Shapes::AlignedBox *)rawFileData;
                    resultValue = S_OK;
                }
                else
                {
                }
            }

            return resultValue;
        }

        HRESULT loadModelWorker(Model &model)
        {
            HRESULT resultValue = E_FAIL;

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(model.fileName, fileData);
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
                    model.alignedBox = *(Shapes::AlignedBox *)rawFileData;
                    rawFileData += sizeof(Shapes::AlignedBox);

                    UINT32 subModelCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    resultValue = S_OK;
                    model.subModelList.resize(subModelCount);
                    for (UINT32 modelIndex = 0; modelIndex < subModelCount; ++modelIndex)
                    {
                        wstring materialName = const wchar_t *(rawFileData);
                        rawFileData += ((materialName.GetLength() + 1) * sizeof(wchar_t));

                        SubModel &subModel = model.subModelList[modelIndex];
                        if (materialName.CompareNoCase(L"skin") == 0)
                        {
                            subModel.skin = true;
                        }
                        else
                        {
                            subModel.material = resources->loadMaterial(materialName);
                            if (!subModel.material)
                            {
                                resultValue = E_FAIL;
                                break;
                            }
                        }

                        UINT32 vertexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        subModel.vertexBuffer = resources->createBuffer(String::format(L"model:vertex:%", &subModel), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, rawFileData);
                        rawFileData += (sizeof(Vertex) * vertexCount);

                        UINT32 indexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        subModel.indexCount = indexCount;
                        subModel.indexBuffer = resources->createBuffer(String::format(L"model:index:%", &subModel), Video::Format::Short, indexCount, Video::BufferType::Index, 0, rawFileData);
                        rawFileData += (sizeof(UINT16) * indexCount);
                    }
                }
            }

            if (SUCCEEDED(resultValue))
            {
                model.loaded = true;
            }

            return resultValue;
        }

        HRESULT loadModel(Model &model)
        {
            std::size_t hash = std::hash<wstring>()(model.fileName);
            if (loadModelSet.count(hash) > 0)
            {
                return S_OK;
            }

            loadModelSet.insert(std::make_pair(hash, true));
            loadModelQueue.push(std::bind(&ModelProcessorImplementation::loadModelWorker, this, std::ref(model)));
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
            GEK_REQUIRE(initializerContext);

            HRESULT resultValue = E_FAIL;
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
                if (!plugin)
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                constantBuffer = resources->createBuffer(nullptr, sizeof(InstanceData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                if (!constantBuffer)
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
            entityDataList.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<ModelComponent, TransformComponent>())
            {
                auto &modelComponent = entity->getComponent<ModelComponent>();
                std::size_t hash = std::hash<wstring>()(modelComponent.value);
                auto pair = dataMap.insert(std::make_pair(hash, Model()));
                if (pair.second)
                {
                    loadBoundingBox(pair.first->second, modelComponent);
                }

                MaterialHandle skinMaterial;
                if (!modelComponent.skin.IsEmpty())
                {
                    skinMaterial = resources->loadMaterial(modelComponent.skin);
                }

                entityDataList.insert(std::make_pair(entity, EntityData(pair.first->second, skinMaterial)));
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto dataEntityIterator = entityDataList.find(entity);
            if (dataEntityIterator != entityDataList.end())
            {
                entityDataList.erase(dataEntityIterator);
            }
        }

        // RenderObserver
        static void drawCall(RenderContext *renderContext, PluginResources *resources, const SubModel &subModel, const InstanceData *instance, ResourceHandle constantBuffer)
        {
            InstanceData *instanceData = nullptr;
            if (SUCCEEDED(resources->mapBuffer(constantBuffer, (void **)&instanceData)))
            {
                memcpy(instanceData, instance, sizeof(InstanceData));
                resources->unmapBuffer(constantBuffer);

                resources->setConstantBuffer(renderContext->vertexPipeline(), constantBuffer, 4);
                resources->setVertexBuffer(renderContext, 0, subModel.vertexBuffer, 0);
                resources->setIndexBuffer(renderContext, subModel.indexBuffer, 0);
                renderContext->getContext()->drawIndexedPrimitive(subModel.indexCount, 0, 0);
            }
        }

        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum)
        {
            GEK_REQUIRE(cameraEntity);
            GEK_REQUIRE(viewFrustum);

            visibleList.clear();
            concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](DataEntityMap::value_type &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                Model &data = dataEntity.second.model;

                const auto &transformComponent = entity->getComponent<TransformComponent>();
                Math::Float4x4 matrix(transformComponent.getMatrix());

                Shapes::OrientedBox orientedBox(data.alignedBox, matrix);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum->isVisible(orientedBox))
                {
                    Gek::Math::Color color(1.0f);
                    if (entity->hasComponent<ColorComponent>())
                    {
                        color = entity->getComponent<ColorComponent>();
                    }

                    auto &materialList = visibleList[&data];
                    auto &instanceList = materialList[dataEntity.second.skin];
                    instanceList.push_back(InstanceData((matrix * *viewMatrix), color, transformComponent.scale));
                }
            });

            concurrency::parallel_for_each(visibleList.begin(), visibleList.end(), [&](auto &visible) -> void
            {
                Model *data = visible.first;
                if (!data->loaded)
                {
                    loadModel(*data);
                    return;
                }

                concurrency::parallel_for_each(visible.second.begin(), visible.second.end(), [&](auto &material) -> void
                {
                    concurrency::parallel_for_each(material.second.begin(), material.second.end(), [&](auto &instance) -> void
                    {
                        concurrency::parallel_for_each(data->subModelList.begin(), data->subModelList.end(), [&](const SubModel &subModel) -> void
                        {
                            render->queueDrawCall(plugin, (subModel.skin ? material.first : subModel.material), std::bind(drawCall, std::placeholders::_1, resources, subModel, &instance, constantBuffer));
                        });
                    });
                });
            });
        }
    };

    REGISTER_CLASS(ModelProcessorImplementation)
}; // namespace Gek

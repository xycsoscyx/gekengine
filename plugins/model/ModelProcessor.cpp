#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Color.h"
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
    namespace Components
    {
        struct Model
        {
            String value;
            String skin;

            Model(void)
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, nullptr, value);
                saveParameter(componentData, L"skin", skin);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                value = loadParameter(componentData, nullptr, String());
                skin = loadParameter(componentData, L"skin", String());
            }
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Model)
        , public Plugin::ComponentMixin<Components::Model>
    {
    public:
        Model(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"model";
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::PopulationObserver
        , public Plugin::RendererObserver
        , public Plugin::Processor
    {
    public:
        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
            Math::Float3 normal;
        };

        struct Material
        {
            bool skin;
            MaterialHandle material;
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            uint32_t indexCount;

            Material(void)
                : skin(false)
                , indexCount(0)
            {
            }

            Material(const Material &material)
                : skin(material.skin)
                , material(material.material)
                , vertexBuffer(material.vertexBuffer)
                , indexBuffer(material.indexBuffer)
                , indexCount(material.indexCount)
            {
            }
        };

        struct Model
        {
            String fileName;
            std::atomic<bool> loaded;
            Shapes::AlignedBox alignedBox;
            std::vector<Material> materialList;

            Model(void)
                : loaded(false)
            {
            }

            Model(const Model &model)
                : fileName(model.fileName)
                , loaded(model.loaded ? true : false)
                , alignedBox(model.alignedBox)
                , materialList(model.materialList)
            {
            }
        };

        struct Data
        {
            Model &model;
            MaterialHandle skin;
            const Math::Color &color;

            Data(Model &model, MaterialHandle skin, const Math::Color &color)
                : model(model)
                , skin(skin)
                , color(color)
            {
            }
        };

        __declspec(align(16))
            struct Instance
        {
            Math::Float4x4 matrix;
            Math::Color color;
            Math::Float3 scale;
            float buffer;

            Instance(const Math::Float4x4 &matrix, const Math::Color &color, const Math::Float3 &scale)
                : matrix(matrix)
                , color(color)
                , scale(scale)
            {
            }
        };

    private:
        Plugin::Population *population;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        VisualHandle visual;
        Video::BufferPtr constantBuffer;

        std::future<void> loadModelRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadModelQueue;
        concurrency::concurrent_unordered_map<std::size_t, bool> loadModelMap;

        std::unordered_map<std::size_t, Model> modelMap;
        using EntityDataMap = std::unordered_map<Plugin::Entity *, Data>;
        EntityDataMap entityDataMap;

        using InstanceList = concurrency::concurrent_vector<Instance, AlignedAllocator<Instance, 16>>;
        using MaterialMap = concurrency::concurrent_unordered_map<MaterialHandle, InstanceList>;
        using VisibleMap = concurrency::concurrent_unordered_map<Model *, MaterialMap>;
        VisibleMap visibleMap;

    public:
        ModelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            population->addObserver(Plugin::PopulationObserver::getObserver());
            renderer->addObserver(Plugin::RendererObserver::getObserver());

            visual = resources->loadVisual(L"model");

            constantBuffer = renderer->getDevice()->createBuffer(sizeof(Instance), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable, false);
        }

        ~ModelProcessor(void)
        {
            renderer->removeObserver(Plugin::RendererObserver::getObserver());
            population->removeObserver(Plugin::PopulationObserver::getObserver());
        }

        void loadBoundingBox(Model &model, const String &modelName)
        {
            static const uint32_t PreReadSize = (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(Shapes::AlignedBox));

            model.fileName = String(L"$root\\data\\models\\%v.gek", modelName);

            std::vector<uint8_t> fileData;
            FileSystem::load(model.fileName, fileData, PreReadSize);

            uint8_t *rawFileData = fileData.data();
            uint32_t gekIdentifier = *((uint32_t *)rawFileData);
            GEK_CHECK_CONDITION(gekIdentifier != *(uint32_t *)"GEKX", Trace::Exception, "Invalid model idetifier found: %v", gekIdentifier);
            rawFileData += sizeof(uint32_t);

            uint16_t gekModelType = *((uint16_t *)rawFileData);
            GEK_CHECK_CONDITION(gekModelType != 0, Trace::Exception, "Invalid model type found: %v", gekModelType);
            rawFileData += sizeof(uint16_t);

            uint16_t gekModelVersion = *((uint16_t *)rawFileData);
            GEK_CHECK_CONDITION(gekModelVersion != 3, Trace::Exception, "Invalid model version found: %v", gekModelVersion);
            rawFileData += sizeof(uint16_t);

            model.alignedBox = *(Shapes::AlignedBox *)rawFileData;
        }

        void loadModelWorker(Model &model)
        {
            std::vector<uint8_t> fileData;
            FileSystem::load(model.fileName, fileData);

            uint8_t *rawFileData = fileData.data();
            uint32_t gekIdentifier = *((uint32_t *)rawFileData);
            GEK_CHECK_CONDITION(gekIdentifier != *(uint32_t *)"GEKX", Trace::Exception, "Invalid model idetifier found: %v", gekIdentifier);
            rawFileData += sizeof(uint32_t);

            uint16_t gekModelType = *((uint16_t *)rawFileData);
            GEK_CHECK_CONDITION(gekModelType != 0, Trace::Exception, "Invalid model type found: %v", gekModelType);
            rawFileData += sizeof(uint16_t);

            uint16_t gekModelVersion = *((uint16_t *)rawFileData);
            GEK_CHECK_CONDITION(gekModelVersion != 3, Trace::Exception, "Invalid model version found: %v", gekModelVersion);
            rawFileData += sizeof(uint16_t);

            model.alignedBox = *(Shapes::AlignedBox *)rawFileData;
            rawFileData += sizeof(Shapes::AlignedBox);

            uint32_t materialCount = *((uint32_t *)rawFileData);
            rawFileData += sizeof(uint32_t);

            model.materialList.resize(materialCount);
            for (uint32_t modelIndex = 0; modelIndex < materialCount; ++modelIndex)
            {
                String materialName((const wchar_t *)rawFileData);
                rawFileData += ((materialName.length() + 1) * sizeof(wchar_t));

                Material &material = model.materialList[modelIndex];
                if (materialName.compareNoCase(L"skin") == 0)
                {
                    material.skin = true;
                }
                else
                {
                    material.material = resources->loadMaterial(materialName);
                }

                uint32_t vertexCount = *((uint32_t *)rawFileData);
                rawFileData += sizeof(uint32_t);

                material.vertexBuffer = resources->createBuffer(String(L"model:vertex:%v:%v", model.fileName, modelIndex), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, std::vector<uint8_t>(rawFileData, rawFileData + (sizeof(Vertex) * vertexCount)));
                rawFileData += (sizeof(Vertex) * vertexCount);

                uint32_t indexCount = *((uint32_t *)rawFileData);
                rawFileData += sizeof(uint32_t);

                material.indexCount = indexCount;
                material.indexBuffer = resources->createBuffer(String(L"model:index:%v:%v", model.fileName, modelIndex), Video::Format::R16_UINT, indexCount, Video::BufferType::Index, 0, std::vector<uint8_t>(rawFileData, rawFileData + (sizeof(uint16_t) * indexCount)));
                rawFileData += (sizeof(uint16_t) * indexCount);
            }

            model.loaded = true;
        }

        void loadModel(Model &model)
        {
            std::size_t hash = std::hash<String>()(model.fileName);
            if (loadModelMap.count(hash) == 0)
            {
                loadModelMap.insert(std::make_pair(hash, true));
                loadModelQueue.push(std::bind(&ModelProcessor::loadModelWorker, this, std::ref(model)));
                if (!loadModelRunning.valid() || (loadModelRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
                {
                    loadModelRunning = std::async(std::launch::async, [&](void) -> void
                    {
                        std::function<void(void)> function;
                        while (loadModelQueue.try_pop(function))
                        {
                            function();
                        };
                    });
                }
            }
        }

        // Plugin::PopulationObserver
        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
            loadModelMap.clear();
            loadModelQueue.clear();
            if (loadModelRunning.valid() && (loadModelRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
            {
                loadModelRunning.get();
            }

            modelMap.clear();
            entityDataMap.clear();
        }

        void onEntityCreated(Plugin::Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<Components::Model, Components::Transform>())
            {
                auto &modelComponent = entity->getComponent<Components::Model>();
                std::size_t hash = std::hash<String>()(modelComponent.value);
                auto pair = modelMap.insert(std::make_pair(hash, Model()));
                if (pair.second)
                {
                    loadBoundingBox(pair.first->second, modelComponent.value);
                }

                MaterialHandle skinMaterial;
                if (!modelComponent.skin.empty())
                {
                    skinMaterial = resources->loadMaterial(modelComponent.skin);
                }

                std::reference_wrapper<const Math::Color> color = Math::Color::White;
                if (entity->hasComponent<Components::Color>())
                {
                    color = std::cref(entity->getComponent<Components::Color>().value);
                }

                Data data(pair.first->second, skinMaterial, color);
                entityDataMap.insert(std::make_pair(entity, data));
            }
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entitySearch = entityDataMap.find(entity);
            if (entitySearch != entityDataMap.end())
            {
                entityDataMap.erase(entitySearch);
            }
        }

        // Plugin::RendererObserver
        static void drawCall(Video::Device::Context *deviceContext, Plugin::Resources *resources, const Material &material, const Instance *instanceList, Video::Buffer *constantBuffer)
        {
            Instance *instanceData = nullptr;
            deviceContext->getDevice()->mapBuffer(constantBuffer, (void **)&instanceData);
            memcpy(instanceData, instanceList, sizeof(Instance));
            deviceContext->getDevice()->unmapBuffer(constantBuffer);

            deviceContext->vertexPipeline()->setConstantBuffer(constantBuffer, 4);
            resources->setVertexBuffer(deviceContext, 0, material.vertexBuffer, 0);
            resources->setIndexBuffer(deviceContext, material.indexBuffer, 0);
            deviceContext->drawIndexedPrimitive(material.indexCount, 0, 0);
        }

        void onRenderScene(Plugin::Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum)
        {
            GEK_TRACE_SCOPE();
            GEK_REQUIRE(renderer);
            GEK_REQUIRE(cameraEntity);
            GEK_REQUIRE(viewFrustum);

            visibleMap.clear();
            concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entityDataPair) -> void
            {
                Plugin::Entity *entity = entityDataPair.first;
                Data &data = entityDataPair.second;
                Model &model = data.model;

                const auto &transformComponent = entity->getComponent<Components::Transform>();
                Math::Float4x4 matrix(transformComponent.getMatrix());

                Shapes::OrientedBox orientedBox(model.alignedBox, matrix);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum->isVisible(orientedBox))
                {
                    auto &materialList = visibleMap[&model];
                    auto &instanceList = materialList[data.skin];
                    instanceList.push_back(Instance((matrix * *viewMatrix), data.color, transformComponent.scale));
                }
            });

            concurrency::parallel_for_each(visibleMap.begin(), visibleMap.end(), [&](auto &visibleMap) -> void
            {
                Model *model = visibleMap.first;
                if (!model->loaded)
                {
                    loadModel(*model);
                    return;
                }

                concurrency::parallel_for_each(visibleMap.second.begin(), visibleMap.second.end(), [&](auto &materialMap) -> void
                {
                    concurrency::parallel_for_each(materialMap.second.begin(), materialMap.second.end(), [&](auto &instanceList) -> void
                    {
                        concurrency::parallel_for_each(model->materialList.begin(), model->materialList.end(), [&](const Material &material) -> void
                        {
                            renderer->queueDrawCall(visual, (material.skin ? materialMap.first : material.material), std::bind(drawCall, std::placeholders::_1, resources, material, &instanceList, constantBuffer.get()));
                        });
                    });
                });
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

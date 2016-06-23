#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Engine.h"
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
        : public ContextRegistration<ModelProcessorImplementation, EngineContext *>
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
        Population *population;
        PluginResources *resources;
        Render *render;

        PluginHandle plugin;
        ResourceHandle constantBuffer;

        std::future<void> loadModelRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadModelQueue;
        concurrency::concurrent_unordered_map<std::size_t, bool> loadModelSet;

        std::unordered_map<std::size_t, Model> modelMap;
        typedef std::unordered_map<Entity *, Data> EntityDataMap;
        EntityDataMap entityDataMap;

        typedef concurrency::concurrent_vector<Instance, AlignedAllocator<Instance, 16>> InstanceList;
        typedef concurrency::concurrent_unordered_map<MaterialHandle, InstanceList> MaterialList;
        typedef concurrency::concurrent_unordered_map<Model *, MaterialList> VisibleList;
        VisibleList visibleList;

    public:
        ModelProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , population(engine->getPopulation())
            , resources(engine->getResources())
            , render(engine->getRender())
        {
            population->addObserver((PopulationObserver *)this);
            render->addObserver((RenderObserver *)this);

            plugin = resources->loadPlugin(L"model");

            constantBuffer = resources->createBuffer(nullptr, sizeof(Instance), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
        }

        ~ModelProcessorImplementation(void)
        {
            render->removeObserver((RenderObserver *)this);
            population->removeObserver((PopulationObserver *)this);
        }

        void loadBoundingBox(Model &model, const String &name)
        {
            static const uint32_t PreReadSize = (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(Shapes::AlignedBox));

            HRESULT resultValue = E_FAIL;

            model.fileName = String(L"$root\\data\\models\\%v.gek", name);

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
                String materialName = (const wchar_t *)rawFileData;
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

                material.vertexBuffer = resources->createBuffer(String(L"model:vertex:%v:%v", model.fileName, modelIndex), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, rawFileData);
                rawFileData += (sizeof(Vertex) * vertexCount);

                uint32_t indexCount = *((uint32_t *)rawFileData);
                rawFileData += sizeof(uint32_t);

                material.indexCount = indexCount;
                material.indexBuffer = resources->createBuffer(String(L"model:index:%v:%v", model.fileName, modelIndex), Video::Format::Short, indexCount, Video::BufferType::Index, 0, rawFileData);
                rawFileData += (sizeof(uint16_t) * indexCount);
            }

            model.loaded = true;
        }

        void loadModel(Model &model)
        {
            std::size_t hash = std::hash<String>()(model.fileName);
            if (loadModelSet.count(hash) == 0)
            {
                loadModelSet.insert(std::make_pair(hash, true));
                loadModelQueue.push(std::bind(&ModelProcessorImplementation::loadModelWorker, this, std::ref(model)));
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

        // PopulationObserver
        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
            loadModelSet.clear();
            loadModelQueue.clear();
            if (loadModelRunning.valid() && (loadModelRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
            {
                loadModelRunning.get();
            }

            modelMap.clear();
            entityDataMap.clear();
        }

        void onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<ModelComponent, TransformComponent>())
            {
                auto &modelComponent = entity->getComponent<ModelComponent>();
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
                if (entity->hasComponent<ColorComponent>())
                {
                    color = std::cref(entity->getComponent<ColorComponent>().value);
                }

                Data data(pair.first->second, skinMaterial, color);
                entityDataMap.insert(std::make_pair(entity, data));
            }
        }

        void onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entityData = entityDataMap.find(entity);
            if (entityData != entityDataMap.end())
            {
                entityDataMap.erase(entityData);
            }
        }

        // RenderObserver
        static void drawCall(RenderContext *renderContext, PluginResources *resources, const Material &material, const Instance *instanceList, ResourceHandle constantBuffer)
        {
            Instance *instanceData = nullptr;
            resources->mapBuffer(constantBuffer, (void **)&instanceData);
            memcpy(instanceData, instanceList, sizeof(Instance));
            resources->unmapBuffer(constantBuffer);

            resources->setConstantBuffer(renderContext->vertexPipeline(), constantBuffer, 4);
            resources->setVertexBuffer(renderContext, 0, material.vertexBuffer, 0);
            resources->setIndexBuffer(renderContext, material.indexBuffer, 0);
            renderContext->drawIndexedPrimitive(material.indexCount, 0, 0);
        }

        void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum)
        {
            GEK_TRACE_SCOPE();
            GEK_REQUIRE(render);
            GEK_REQUIRE(cameraEntity);
            GEK_REQUIRE(viewFrustum);

            visibleList.clear();
            concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](EntityDataMap::value_type &data) -> void
            {
                Entity *entity = data.first;
                Model &model = data.second.model;

                const auto &transformComponent = entity->getComponent<TransformComponent>();
                Math::Float4x4 matrix(transformComponent.getMatrix());

                Shapes::OrientedBox orientedBox(model.alignedBox, matrix);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum->isVisible(orientedBox))
                {
                    auto &materialList = visibleList[&model];
                    auto &instanceList = materialList[data.second.skin];
                    instanceList.push_back(Instance((matrix * *viewMatrix), data.second.color, transformComponent.scale));
                }
            });

            concurrency::parallel_for_each(visibleList.begin(), visibleList.end(), [&](auto &visibleList) -> void
            {
                Model *model = visibleList.first;
                if (!model->loaded)
                {
                    loadModel(*model);
                    return;
                }

                concurrency::parallel_for_each(visibleList.second.begin(), visibleList.second.end(), [&](auto &materialList) -> void
                {
                    concurrency::parallel_for_each(materialList.second.begin(), materialList.second.end(), [&](auto &instanceList) -> void
                    {
                        concurrency::parallel_for_each(model->materialList.begin(), model->materialList.end(), [&](const Material &material) -> void
                        {
                            render->queueDrawCall(plugin, (material.skin ? materialList.first : material.material), std::bind(drawCall, std::placeholders::_1, resources, material, &instanceList, constantBuffer));
                        });
                    });
                });
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(ModelProcessorImplementation)
}; // namespace Gek

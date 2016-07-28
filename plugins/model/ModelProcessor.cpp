#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\ContextUser.h"
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
#include <future>
#include <algorithm>
#include <memory>
#include <array>
#include <map>

namespace Gek
{
    namespace Components
    {
        struct Model
        {
            String name;
            String skin;

            Model(void)
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, nullptr, name);
                saveParameter(componentData, L"skin", skin);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                name = loadParameter(componentData, nullptr, String());
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
        , public Plugin::PopulationListener
        , public Plugin::RendererListener
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
            std::mutex mutex;
            std::shared_future<void> loadBox;
            std::shared_future<void> loadData;
            std::function<void(Model &)> load;

            Shapes::AlignedBox alignedBox;
            std::vector<Material> materialList;

            Model(void)
            {
            }

            Model(const Model &model)
                : loadBox(model.loadBox)
                , loadData(model.loadData)
                , load(model.load)
                , alignedBox(model.alignedBox)
                , materialList(model.materialList)
            {
            }

            bool valid(void)
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!loadData.valid())
                {
                    loadData = std::async(std::launch::async, [&](void) -> void
                    {
                        load(*this);
                    });
                }

                return (loadData.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
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

        concurrency::concurrent_unordered_map<std::size_t, Model> modelMap;
        using EntityDataMap = concurrency::concurrent_unordered_map<Plugin::Entity *, Data>;
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
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->addListener(this);
            renderer->addListener(this);

            visual = resources->loadVisual(L"model");

            constantBuffer = renderer->getDevice()->createBuffer(sizeof(Instance), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable, false);
        }

        ~ModelProcessor(void)
        {
            renderer->removeListener(this);
            population->removeListener(this);
        }

        // Plugin::PopulationListener
        void onLoadBegin(void)
        {
            modelMap.clear();
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

            if (entity->hasComponents<Components::Model, Components::Transform>())
            {
                auto &modelComponent = entity->getComponent<Components::Model>();
                std::size_t hash = std::hash<String>()(modelComponent.name);
                String fileName(L"$root\\data\\models\\%v.gek", modelComponent.name);

                auto pair = modelMap.insert(std::make_pair(hash, Model()));
                if (pair.second)
                {
                    pair.first->second.loadBox = std::async(std::launch::async, [this, name = String(modelComponent.name), fileName, alignedBox = &pair.first->second.alignedBox](void) -> void
                    {
                        static const uint32_t PreReadSize = (sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(Shapes::AlignedBox));

                        std::vector<uint8_t> fileData;
                        FileSystem::load(fileName, fileData, PreReadSize);

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

                        (*alignedBox) = *(Shapes::AlignedBox *)rawFileData;
                    }).share();

                    pair.first->second.load = [this, name = String(modelComponent.name), fileName](Model &model) -> void
                    {
                        std::vector<uint8_t> fileData;
                        FileSystem::load(fileName, fileData);

                        uint8_t *rawFileData = fileData.data();
                        rawFileData += sizeof(uint32_t);
                        rawFileData += sizeof(uint16_t);
                        rawFileData += sizeof(uint16_t);
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

                            material.vertexBuffer = resources->createBuffer(String(L"model:vertex:%v:%v", name, modelIndex), sizeof(Vertex), vertexCount, Video::BufferType::Vertex, 0, std::vector<uint8_t>(rawFileData, rawFileData + (sizeof(Vertex) * vertexCount)));
                            rawFileData += (sizeof(Vertex) * vertexCount);

                            uint32_t indexCount = *((uint32_t *)rawFileData);
                            rawFileData += sizeof(uint32_t);

                            material.indexCount = indexCount;
                            material.indexBuffer = resources->createBuffer(String(L"model:index:%v:%v", name, modelIndex), Video::Format::R16_UINT, indexCount, Video::BufferType::Index, 0, std::vector<uint8_t>(rawFileData, rawFileData + (sizeof(uint16_t) * indexCount)));
                            rawFileData += (sizeof(uint16_t) * indexCount);
                        }
                    };
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
                entityDataMap.unsafe_erase(entitySearch);
            }
        }

        // Plugin::RendererListener
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

        void onRenderScene(Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
        {
            GEK_TRACE_SCOPE();
            GEK_REQUIRE(renderer);
            GEK_REQUIRE(cameraEntity);

            visibleMap.clear();
            concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entityDataPair) -> void
            {
                Plugin::Entity *entity = entityDataPair.first;
                Data &data = entityDataPair.second;
                Model &model = data.model;
                if (model.loadBox.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    const auto &transformComponent = entity->getComponent<Components::Transform>();
                    Math::Float4x4 matrix(transformComponent.getMatrix());

                    Shapes::OrientedBox orientedBox(model.alignedBox, matrix);
                    orientedBox.halfsize *= transformComponent.scale;

                    if (viewFrustum.isVisible(orientedBox))
                    {
                        auto &materialList = visibleMap[&model];
                        auto &instanceList = materialList[data.skin];
                        instanceList.push_back(Instance((matrix * viewMatrix), data.color, transformComponent.scale));
                    }
                }
            });

            concurrency::parallel_for_each(visibleMap.begin(), visibleMap.end(), [&](auto &visibleMap) -> void
            {
                Model &model = *visibleMap.first;
                if (model.valid() && !model.materialList.empty())
                {
                    concurrency::parallel_for_each(visibleMap.second.begin(), visibleMap.second.end(), [&](auto &materialMap) -> void
                    {
                        concurrency::parallel_for_each(materialMap.second.begin(), materialMap.second.end(), [&](auto &instanceList) -> void
                        {
                            concurrency::parallel_for_each(model.materialList.begin(), model.materialList.end(), [&](const Material &material) -> void
                            {
                                renderer->queueDrawCall(visual, (material.skin ? materialMap.first : material.material), std::bind(drawCall, std::placeholders::_1, resources, material, &instanceList, constantBuffer.get()));
                            });
                        });
                    });
                }
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

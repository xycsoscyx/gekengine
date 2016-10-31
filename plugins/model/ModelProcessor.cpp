#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Shapes\AlignedBox.hpp"
#include "GEK\Shapes\OrientedBox.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ThreadPool.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\Allocator.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Components\Color.hpp"
#include "GEK\Model\Base.hpp"
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
        void Model::save(Xml::Leaf &componentData) const
        {
            componentData.text = name;
            componentData.attributes[L"skin"] = skin;
        }

        void Model::load(const Xml::Leaf &componentData)
        {
            name = componentData.text;
            skin = componentData.getAttribute(L"skin");
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Model)
        , public Plugin::ComponentMixin<Components::Model, Edit::Component>
    {
    public:
        Model(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &modelComponent = *dynamic_cast<Components::Model *>(data);
            ImGui::InputText("Model", modelComponent.name, flags);
            ImGui::InputText("Skin", modelComponent.skin, flags);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, 0);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"model";
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<ModelProcessor, Components::Model, Components::Transform>
        , public Plugin::Processor
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(InvalidModelIdentifier);
        GEK_ADD_EXCEPTION(InvalidModelType);
        GEK_ADD_EXCEPTION(InvalidModelVersion);

    public:
        struct Header
        {
            struct Material
            {
                wchar_t name[64];
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;

            Shapes::AlignedBox boundingBox;

            uint32_t materialCount = 0;
            Material materialList[1];
        };

        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
			Math::Float3 tangent;
			Math::Float3 biTangent;
			Math::Float3 normal;
        };

        struct Material
        {
            bool skin = false;
            MaterialHandle material;
            ResourceHandle vertexBuffer;
            ResourceHandle indexBuffer;
            uint32_t indexCount = 0;
        };

        struct Model
        {
            Shapes::AlignedBox alignedBox;
            std::vector<Material> materialList;
        };

        struct Data
        {
            Model *model = nullptr;
            MaterialHandle skin;
        };

        struct Instance
        {
            Math::SIMD::Float4x4 matrix;

            Instance(const Math::SIMD::Float4x4 &matrix)
                : matrix(matrix)
            {
            }
        };

    private:
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;

        VisualHandle visual;
        Video::BufferPtr constantBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Model> modelMap;

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
            , loadPool(1)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->onLoadBegin.connect<ModelProcessor, &ModelProcessor::onLoadBegin>(this);
            population->onLoadSucceeded.connect<ModelProcessor, &ModelProcessor::onLoadSucceeded>(this);
            population->onEntityCreated.connect<ModelProcessor, &ModelProcessor::onEntityCreated>(this);
            population->onEntityDestroyed.connect<ModelProcessor, &ModelProcessor::onEntityDestroyed>(this);
            population->onComponentAdded.connect<ModelProcessor, &ModelProcessor::onComponentAdded>(this);
            population->onComponentRemoved.connect<ModelProcessor, &ModelProcessor::onComponentRemoved>(this);
            renderer->onRenderScene.connect<ModelProcessor, &ModelProcessor::onRenderScene>(this);

            visual = resources->loadVisual(L"model");

            constantBuffer = renderer->getVideoDevice()->createBuffer(sizeof(Instance), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
        }

        ~ModelProcessor(void)
        {
            renderer->onRenderScene.disconnect<ModelProcessor, &ModelProcessor::onRenderScene>(this);
            population->onComponentRemoved.disconnect<ModelProcessor, &ModelProcessor::onComponentRemoved>(this);
            population->onComponentAdded.disconnect<ModelProcessor, &ModelProcessor::onComponentAdded>(this);
            population->onEntityDestroyed.disconnect<ModelProcessor, &ModelProcessor::onEntityDestroyed>(this);
            population->onEntityCreated.disconnect<ModelProcessor, &ModelProcessor::onEntityCreated>(this);
            population->onLoadSucceeded.disconnect<ModelProcessor, &ModelProcessor::onLoadSucceeded>(this);
            population->onLoadBegin.disconnect<ModelProcessor, &ModelProcessor::onLoadBegin>(this);
        }

        template <typename TYPE>
        std::vector<uint8_t> getBuffer(uint8_t **bufferData, uint32_t count)
        {
            auto start = (*bufferData);
            (*bufferData) += (sizeof(TYPE) * count);
            auto end = (*bufferData);
            return std::vector<uint8_t>(start, end);
        }

        void addEntity(Plugin::Entity *entity)
        {
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                String fileName(getContext()->getFileName(L"data\\models", modelComponent.name).append(L".gek"));
                auto pair = modelMap.insert(std::make_pair(getHash(modelComponent.name), Model()));
                if (pair.second)
                {
                    loadPool.enqueue([this, name = String(modelComponent.name), fileName, &model = pair.first->second](void) -> void
                    {
                        std::vector<uint8_t> buffer;
                        FileSystem::load(fileName, buffer, sizeof(Header));

                        Header *header = (Header *)buffer.data();
                        if (header->identifier != *(uint32_t *)"GEKX")
                        {
                            throw InvalidModelIdentifier();
                        }

                        if (header->type != 0)
                        {
                            throw InvalidModelType();
                        }

                        if (header->version != 5)
                        {
                            throw InvalidModelVersion();
                        }

                        model.alignedBox = header->boundingBox;
                        loadPool.enqueue([this, &model, name, fileName](void) -> void
                        {
                            std::vector<uint8_t> buffer;
                            FileSystem::load(fileName, buffer);

                            Header *header = (Header *)buffer.data();
                            model.materialList.resize(header->materialCount);
                            uint8_t *bufferData = (uint8_t *)&header->materialList[header->materialCount];
                            for (uint32_t materialIndex = 0; materialIndex < header->materialCount; ++materialIndex)
                            {
                                Header::Material &materialHeader = header->materialList[materialIndex];
                                Material &material = model.materialList[materialIndex];
                                if (wcsicmp(materialHeader.name, L"skin") == 0)
                                {
                                    material.skin = true;
                                }
                                else
                                {
                                    material.material = resources->loadMaterial(materialHeader.name);
                                }

                                material.indexCount = materialHeader.indexCount;
                                material.indexBuffer = resources->createBuffer(String::create(L"model:index:%v:%v", name, materialIndex), Video::Format::R16_UINT, materialHeader.indexCount, Video::BufferType::Index, 0, getBuffer<uint16_t>(&bufferData, materialHeader.indexCount));
                                material.vertexBuffer = resources->createBuffer(String::create(L"model:vertex:%v:%v", name, materialIndex), sizeof(Vertex), materialHeader.vertexCount, Video::BufferType::Vertex, 0, getBuffer<Vertex>(&bufferData, materialHeader.vertexCount));
                            }
                        });
                    });
                }

                data.skin = resources->loadMaterial(modelComponent.skin);
                data.model = &pair.first->second;
            });
        }

        // Plugin::Population Slots
        void onLoadBegin(const String &populationName)
        {
            loadPool.clear();
            modelMap.clear();
            clear();
        }

        void onLoadSucceeded(const String &populationName)
        {
            population->listEntities([&](Plugin::Entity *entity, const wchar_t *) -> void
            {
                addEntity(entity);
            });
        }

        void onEntityCreated(Plugin::Entity *entity, const wchar_t *entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity *entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity *entity, const std::type_index &type)
        {
            removeEntity(entity);
        }

        // Plugin::Renderer Slots
        static void drawCall(Video::Device *videoDevice, Video::Device::Context *videoContext, Plugin::Resources *resources, const Material &material, const Instance *instanceList, Video::Buffer *constantBuffer)
        {
            Instance *instanceData = nullptr;
            videoDevice->mapBuffer(constantBuffer, instanceData);
            std::copy(instanceList, (instanceList + 1), instanceData);
            videoDevice->unmapBuffer(constantBuffer);

            videoContext->vertexPipeline()->setConstantBufferList({ constantBuffer }, 4);
            resources->setVertexBufferList(videoContext, { material.vertexBuffer }, 0);
            resources->setIndexBuffer(videoContext, material.indexBuffer, 0);
            resources->drawIndexedPrimitive(videoContext, material.indexCount, 0, 0);
        }

        void onRenderScene(const Shapes::Frustum &viewFrustum, const Math::SIMD::Float4x4 &viewMatrix)
        {
            GEK_REQUIRE(renderer);

            visibleMap.clear();
            list([&](Plugin::Entity *entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                Model &model = *data.model;
                Math::SIMD::Float4x4 matrix(transformComponent.getMatrix());

                Shapes::OrientedBox orientedBox(model.alignedBox, matrix);
                orientedBox.halfsize *= transformComponent.scale;

                if (viewFrustum.isVisible(orientedBox))
                {
                    auto &materialList = visibleMap[&model];
                    auto &instanceList = materialList[data.skin];
                    instanceList.push_back(Instance(matrix * viewMatrix));
                }
            });

            concurrency::parallel_for_each(visibleMap.begin(), visibleMap.end(), [&](auto &visibleMap) -> void
            {
                auto model = visibleMap.first;
                if (!model->materialList.empty())
                {
                    concurrency::parallel_for_each(visibleMap.second.begin(), visibleMap.second.end(), [&](auto &materialMap) -> void
                    {
                        concurrency::parallel_for_each(materialMap.second.begin(), materialMap.second.end(), [&](auto &instanceList) -> void
                        {
                            concurrency::parallel_for_each(model->materialList.begin(), model->materialList.end(), [&](const Material &material) -> void
                            {
                                renderer->queueDrawCall(visual, (material.skin ? materialMap.first : material.material), std::bind(drawCall, renderer->getVideoDevice(), std::placeholders::_1, resources, material, &instanceList, constantBuffer.get()));
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

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Allocator.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <algorithm>
#include <memory>
#include <future>
#include <ppl.h>
#include <array>
#include <map>

namespace Gek
{
    GEK_CONTEXT_USER(Model, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Model, Edit::Component>
    {
    public:
        Model(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(const Components::Model *data, JSON::Object &componentData) const
        {
            componentData.set(L"name", data->name);
            componentData.set(L"skin", data->skin);
        }

        void load(Components::Model *data, const JSON::Object &componentData)
        {
            data->name = getValue(componentData, L"name", String());
            data->skin = getValue(componentData, L"skin", String());
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

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, 0);
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<ModelProcessor, Components::Model, Components::Transform>
        , public Plugin::Processor
    {
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

    private:
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;

        VisualHandle visual;
        Video::BufferPtr constantBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Model> modelMap;

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

            Video::BufferDescription description;
            description.stride = sizeof(Math::Float4x4);
            description.count = 1;
            description.type = Video::BufferDescription::Type::Constant;
            description.flags = Video::BufferDescription::Flags::Mappable;
            constantBuffer = renderer->getVideoDevice()->createBuffer(description);
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

        void addEntity(Plugin::Entity *entity)
        {
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                String fileName(getContext()->getFileName(L"data\\models", modelComponent.name).append(L".gek"));
                auto pair = modelMap.insert(std::make_pair(GetHash(modelComponent.name), Model()));
                if (pair.second)
                {
                    loadPool.enqueue([this, name = modelComponent.name, fileName, &model = pair.first->second](void) -> void
                    {
                        std::vector<uint8_t> buffer;
                        FileSystem::Load(fileName, buffer, sizeof(Header));

                        Header *header = (Header *)buffer.data();
                        if (header->identifier != *(uint32_t *)"GEKX")
                        {
                            throw InvalidModelIdentifier("Unknown model file identifier encountered");
                        }

                        if (header->type != 0)
                        {
                            throw InvalidModelType("Unsupported model type encountered");
                        }

                        if (header->version != 5)
                        {
                            throw InvalidModelVersion("Unsupported model version encountered");
                        }

                        model.alignedBox = header->boundingBox;
                        loadPool.enqueue([this, &model, name, fileName](void) -> void
                        {
                            std::vector<uint8_t> buffer;
                            FileSystem::Load(fileName, buffer);

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

                                Video::BufferDescription indexBufferDescription;
                                indexBufferDescription.format = Video::Format::R16_UINT;
                                indexBufferDescription.count = materialHeader.indexCount;
                                indexBufferDescription.type = Video::BufferDescription::Type::Index;
                                material.indexBuffer = resources->createBuffer(String::Format(L"model:index:%v:%v", name, materialIndex), indexBufferDescription, reinterpret_cast<uint16_t *>(bufferData));
                                bufferData += (sizeof(uint16_t) * materialHeader.indexCount);

                                Video::BufferDescription vertexBufferDescription;
                                vertexBufferDescription.stride = sizeof(Vertex);
                                vertexBufferDescription.count = materialHeader.vertexCount;
                                vertexBufferDescription.type = Video::BufferDescription::Type::Vertex;
                                material.vertexBuffer = resources->createBuffer(String::Format(L"model:vertex:%v:%v", name, materialIndex), vertexBufferDescription, reinterpret_cast<Vertex *>(bufferData));
                                bufferData += (sizeof(Vertex) * materialHeader.vertexCount);

                                material.indexCount = materialHeader.indexCount;
                            }
                        });
                    });
                }

                if (!modelComponent.skin.empty())
                {
                    data.skin = resources->loadMaterial(modelComponent.skin);
                }

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
        void onRenderScene(const Shapes::Frustum &viewFrustum, const Math::Float4x4 &viewMatrix)
        {
            GEK_REQUIRE(renderer);

            list([&](Plugin::Entity *entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                Model &model = *data.model;
                Math::Float4x4 matrix(transformComponent.getMatrix());
                Shapes::OrientedBox orientedBox(model.alignedBox, matrix);
                if (viewFrustum.isVisible(orientedBox))
                {
                    auto modelViewMatrix(matrix * viewMatrix);
                    concurrency::parallel_for_each(std::begin(model.materialList), std::end(model.materialList), [&](const Material &material) -> void
                    {
                        renderer->queueDrawCall(visual, (material.skin ? data.skin : material.material), [this, modelViewMatrix, material](Video::Device::Context *videoContext) -> void
                        {
                            Math::Float4x4 *constantData = nullptr;
                            if (renderer->getVideoDevice()->mapBuffer(constantBuffer.get(), constantData))
                            {
                                memcpy(constantData, &modelViewMatrix, sizeof(Math::Float4x4));
                                renderer->getVideoDevice()->unmapBuffer(constantBuffer.get());

                                videoContext->vertexPipeline()->setConstantBufferList({ constantBuffer.get() }, 4);
                                resources->setVertexBufferList(videoContext, { material.vertexBuffer }, 0);
                                resources->setIndexBuffer(videoContext, material.indexBuffer, 0);
                                resources->drawIndexedPrimitive(videoContext, material.indexCount, 0, 0);
                            }
                        });
                    });
                }
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

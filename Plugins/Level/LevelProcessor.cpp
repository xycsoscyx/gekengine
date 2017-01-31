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
#include "GEK/Level/Base.hpp"
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
    GEK_CONTEXT_USER(Level, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Level, Edit::Component>
    {
    public:
        Level(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Level const * const data, JSON::Object &componentData) const
        {
            componentData.set(L"name", data->name);
        }

        void load(Components::Level * const data, const JSON::Object &componentData)
        {
            data->name = getValue(componentData, L"name", String());
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &levelComponent = *dynamic_cast<Components::Level *>(data);
            bool changed = ImGui::Gek::InputString("Level", levelComponent.name, flags);
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, 0);
        }
    };

    GEK_CONTEXT_USER(LevelProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<LevelProcessor, Components::Level, Components::Transform>
        , public Plugin::Processor
    {
        GEK_ADD_EXCEPTION(InvalidLevelIdentifier);
        GEK_ADD_EXCEPTION(InvalidLevelType);
        GEK_ADD_EXCEPTION(InvalidLevelVersion);

    public:
        struct Header
        {
            struct Part
            {
                wchar_t name[64];
                Shapes::AlignedBox boundingBox;
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;

            uint32_t instanceCount;
            uint32_t partIndexCount;
            uint32_t partCount;
        };

        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
			Math::Float3 tangent;
			Math::Float3 biTangent;
			Math::Float3 normal;
        };

        struct Level
        {
            struct Instance
            {
                Math::Float4x4 transform;
                Math::Float3 scale;
                uint32_t partIndexStart;
                uint32_t partIndexCount;
            };

            struct Part
            {
                MaterialHandle material;
                Shapes::AlignedBox boundingBox;
                std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(5);
                ResourceHandle indexBuffer;
                uint32_t indexCount = 0;
            };

            std::vector<Instance> instanceList;
            std::vector<uint32_t> partIndexList;
            std::vector<Part> partList;
        };

        struct Data
        {
            Level *level = nullptr;
        };

    private:
        Video::Device *videoDevice = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;

        VisualHandle visual;
        Video::BufferPtr instanceBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Level> levelMap;

    public:
        LevelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , videoDevice(core->getVideoDevice())
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
            , loadPool(1)
        {
            GEK_REQUIRE(videoDevice);
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->onLoadBegin.connect<LevelProcessor, &LevelProcessor::onLoadBegin>(this);
            population->onLoadSucceeded.connect<LevelProcessor, &LevelProcessor::onLoadSucceeded>(this);
            population->onEntityCreated.connect<LevelProcessor, &LevelProcessor::onEntityCreated>(this);
            population->onEntityDestroyed.connect<LevelProcessor, &LevelProcessor::onEntityDestroyed>(this);
            population->onComponentAdded.connect<LevelProcessor, &LevelProcessor::onComponentAdded>(this);
            population->onComponentRemoved.connect<LevelProcessor, &LevelProcessor::onComponentRemoved>(this);
            renderer->onRenderScene.connect<LevelProcessor, &LevelProcessor::onRenderScene>(this);

            visual = resources->loadVisual(L"level");

            Video::Buffer::Description instanceDescription;
            instanceDescription.stride = sizeof(Math::Float4x4);
            instanceDescription.count = 100;
            instanceDescription.type = Video::Buffer::Description::Type::Vertex;
            instanceDescription.flags = Video::Buffer::Description::Flags::Mappable;
            instanceBuffer = videoDevice->createBuffer(instanceDescription);
            instanceBuffer->setName(L"level:instances");
        }

        ~LevelProcessor(void)
        {
            renderer->onRenderScene.disconnect<LevelProcessor, &LevelProcessor::onRenderScene>(this);
            population->onComponentRemoved.disconnect<LevelProcessor, &LevelProcessor::onComponentRemoved>(this);
            population->onComponentAdded.disconnect<LevelProcessor, &LevelProcessor::onComponentAdded>(this);
            population->onEntityDestroyed.disconnect<LevelProcessor, &LevelProcessor::onEntityDestroyed>(this);
            population->onEntityCreated.disconnect<LevelProcessor, &LevelProcessor::onEntityCreated>(this);
            population->onLoadSucceeded.disconnect<LevelProcessor, &LevelProcessor::onLoadSucceeded>(this);
            population->onLoadBegin.disconnect<LevelProcessor, &LevelProcessor::onLoadBegin>(this);
        }

        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &levelComponent, auto &transformComponent) -> void
            {
                String fileName(getContext()->getRootFileName(L"data", L"models", levelComponent.name).withExtension(L".gek"));
                auto pair = levelMap.insert(std::make_pair(GetHash(levelComponent.name), Level()));
                if (pair.second)
                {
                    loadPool.enqueue([this, name = levelComponent.name, fileName, &level = pair.first->second](void) -> void
                    {
                        std::vector<uint8_t> buffer;
                        FileSystem::Load(fileName, buffer, sizeof(Header));

                        Header *header = (Header *)buffer.data();
                        if (header->identifier != *(uint32_t *)"GEKX")
                        {
                            throw InvalidLevelIdentifier("Unknown level file identifier encountered");
                        }

                        if (header->type != 3)
                        {
                            throw InvalidLevelType("Unsupported level type encountered");
                        }

                        if (header->version != 1)
                        {
                            throw InvalidLevelVersion("Unsupported level version encountered");
                        }

                        loadPool.enqueue([this, name = name, fileName, &level](void) -> void
                        {
                            std::vector<uint8_t> buffer;
                            FileSystem::Load(fileName, buffer);

                            Header *header = (Header *)buffer.data();

                            uint8_t *bufferData = (uint8_t *)&header[1];
                            auto instanceData = (Level::Instance *)bufferData;
                            level.instanceList.resize(header->instanceCount);
                            std::copy(instanceData, &instanceData[header->instanceCount], level.instanceList.data());

                            bufferData = (uint8_t *)&instanceData[header->instanceCount];
                            auto partIndexData = (uint32_t *)bufferData;
                            level.partIndexList.resize(header->partIndexCount);
                            std::copy(partIndexData, &partIndexData[header->partIndexCount], level.partIndexList.data());

                            level.partList.resize(header->partCount);
                            bufferData = (uint8_t *)&partIndexData[header->partIndexCount];
                            auto partData = (Header::Part *)bufferData;
                            bufferData = (uint8_t *)&partData[header->partCount];
                            for (uint32_t partIndex = 0; partIndex < header->partCount; ++partIndex)
                            {
                                const Header::Part &partHeader = partData[partIndex];
                                Level::Part &part = level.partList[partIndex];
                                part.material = resources->loadMaterial(partHeader.name);
                                part.boundingBox = partHeader.boundingBox;

                                Video::Buffer::Description indexBufferDescription;
                                indexBufferDescription.format = Video::Format::R16_UINT;
                                indexBufferDescription.count = partHeader.indexCount;
                                indexBufferDescription.type = Video::Buffer::Description::Type::Index;
                                part.indexBuffer = resources->createBuffer(String::Format(L"level:indices:%v:%v", name, partIndex), indexBufferDescription, reinterpret_cast<uint16_t *>(bufferData));
                                bufferData += (sizeof(uint16_t) * partHeader.indexCount);

                                Video::Buffer::Description vertexBufferDescription;
                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                vertexBufferDescription.count = partHeader.vertexCount;
                                vertexBufferDescription.type = Video::Buffer::Description::Type::Vertex;
                                part.vertexBufferList[0] = resources->createBuffer(String::Format(L"level:positions:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float2);
                                part.vertexBufferList[1] = resources->createBuffer(String::Format(L"level:texcoords:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float2 *>(bufferData));
                                bufferData += (sizeof(Math::Float2) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[2] = resources->createBuffer(String::Format(L"level:tangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[3] = resources->createBuffer(String::Format(L"level:bitangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[4] = resources->createBuffer(String::Format(L"level:normals:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                part.indexCount = partHeader.indexCount;
                            }
                        });
                    });
                }

                data.level = &pair.first->second;
            });
        }

        // Plugin::Population Slots
        void onLoadBegin(String const &populationName)
        {
            loadPool.clear();
            levelMap.clear();
            clear();
        }

        void onLoadSucceeded(String const &populationName)
        {
            population->listEntities([&](Plugin::Entity * const entity, wchar_t const * const ) -> void
            {
                addEntity(entity);
            });
        }

        void onEntityCreated(Plugin::Entity * const entity, wchar_t const * const entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity, const std::type_index &type)
        {
            removeEntity(entity);
        }

        using InstanceList = concurrency::concurrent_vector<Math::Float4x4>;
        using PartMap = concurrency::concurrent_unordered_map<const Level::Part *, InstanceList>;
        using MaterialMap = concurrency::concurrent_unordered_map<MaterialHandle, PartMap>;

        // Plugin::Renderer Slots
        void onRenderScene(const Shapes::Frustum &viewFrustum, Math::Float4x4 const &viewMatrix)
        {
            GEK_REQUIRE(renderer);

            MaterialMap materialMap;
            list([&](Plugin::Entity * const entity, auto &data, auto &levelComponent, auto &transformComponent) -> void
            {
                Level &level = *data.level;
                Math::Float4x4 matrix(transformComponent.getMatrix());
                for (auto &instance : level.instanceList)
                {
                    auto scale = instance.transform.getScaling();
                    auto transform(matrix * instance.transform);
                    for (uint32_t partIndexIndex = 0; partIndexIndex < instance.partIndexCount; partIndexIndex++)
                    {
                        auto partIndex = level.partIndexList[partIndexIndex];
                        auto &part = level.partList[partIndex];
                        Shapes::OrientedBox orientedBox(part.boundingBox, transform);
                        orientedBox.halfsize *= instance.scale;
                        if (viewFrustum.isVisible(orientedBox))
                        {
                            auto levelViewMatrix(transform * viewMatrix);
                            concurrency::parallel_for_each(std::begin(level.partList), std::end(level.partList), [&](const Level::Part &part) -> void
                            {
                                auto &partMap = materialMap[part.material];
                                auto &instanceList = partMap[&part];
                                instanceList.push_back(levelViewMatrix);
                            });
                        }
                    }
                }
            });

            size_t maximumInstanceCount = 0;
            for (auto &materialPair : materialMap)
            {
                auto material = materialPair.first;
                auto &partMap = materialPair.second;

                struct DrawData
                {
                    uint32_t instanceStart = 0;
                    uint32_t instanceCount = 0;
                    const Level::Part *part = nullptr;

                    DrawData(uint32_t instanceStart = 0, uint32_t instanceCount = 0, const Level::Part *part = nullptr)
                        : instanceStart(instanceStart)
                        , instanceCount(instanceCount)
                        , part(part)
                    {
                    }
                };

                std::vector<Math::Float4x4> instanceList;
                std::vector<DrawData> drawDataList;

                for (auto &partPair : partMap)
                {
                    auto part = partPair.first;
                    auto &partInstanceList = partPair.second;
                    drawDataList.push_back(DrawData(instanceList.size(), partInstanceList.size(), part));
                    instanceList.insert(std::end(instanceList), std::begin(partInstanceList), std::end(partInstanceList));
                }

                maximumInstanceCount = std::max(maximumInstanceCount, instanceList.size());
                renderer->queueDrawCall(visual, material, std::move([this, drawDataList = move(drawDataList), instanceList = move(instanceList)](Video::Device::Context *videoContext) -> void
                {
                    Math::Float4x4 *instanceData = nullptr;
                    if (videoDevice->mapBuffer(instanceBuffer.get(), instanceData))
                    {
                        std::copy(std::begin(instanceList), std::end(instanceList), instanceData);
                        videoDevice->unmapBuffer(instanceBuffer.get());

                        videoContext->setVertexBufferList({ instanceBuffer.get() }, 5);

                        for (auto &drawData : drawDataList)
                        {
                            resources->setVertexBufferList(videoContext, drawData.part->vertexBufferList, 0);
                            resources->setIndexBuffer(videoContext, drawData.part->indexBuffer, 0);
                            resources->drawInstancedIndexedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, drawData.part->indexCount, 0, 0);
                        }
                    }
                }));
            }

            if (instanceBuffer->getDescription().count < maximumInstanceCount)
            {
                instanceBuffer = nullptr;
                Video::Buffer::Description instanceDescription;
                instanceDescription.stride = sizeof(Math::Float4x4);
                instanceDescription.count = maximumInstanceCount;
                instanceDescription.type = Video::Buffer::Description::Type::Vertex;
                instanceDescription.flags = Video::Buffer::Description::Flags::Mappable;
                instanceBuffer = videoDevice->createBuffer(instanceDescription);
                instanceBuffer->setName(L"level:instances");
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Level)
    GEK_REGISTER_CONTEXT_USER(LevelProcessor)
}; // namespace Gek

﻿#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/SIMD.hpp"
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
#include "GEK/Engine/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <xmmintrin.h>
#include <algorithm>
#include <memory>
#include <future>
#include <ppl.h>
#include <array>
#include <map>
#include <set>

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
        void save(Components::Model const * const data, JSON::Object &componentData) const
        {
            componentData = data->name;
        }

        void load(Components::Model * const data, JSON::Reference componentData)
        {
            data->name = parse(componentData, String::Empty);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &modelComponent = *dynamic_cast<Components::Model *>(data);

            changed |= editorElement("Model", [&](void) -> bool
            {
                return UI::InputString("##model", modelComponent.name, 0);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<ModelProcessor, Components::Model, Components::Transform>
        , public Plugin::Processor
        , public Gek::Processor::Model
    {
    public:
        struct Header
        {
            struct Part
            {
                char name[64];
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;

            Shapes::AlignedBox boundingBox;

            uint32_t partCount = 0;
            Part partList[1];
        };

        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
			Math::Float3 tangent;
			Math::Float3 biTangent;
			Math::Float3 normal;
        };

        struct Model
        {
            struct Part
            {
                MaterialHandle material;
                std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(5);
                ResourceHandle indexBuffer;
                uint32_t indexCount = 0;
            };

            Shapes::AlignedBox boundingBox;
            std::vector<Part> partList;
        };

        struct Data
        {
            Model *model = nullptr;
        };

        struct Instance
        {
            Math::Float4 color = Math::Float4::White;
            Math::Float4x4 transform = Math::Float4x4::Identity;
        };

        struct DrawData
        {
            uint32_t instanceStart = 0;
            uint32_t instanceCount = 0;
            const Model::Part *part = nullptr;

            DrawData(uint32_t instanceStart = 0, uint32_t instanceCount = 0, const Model::Part *part = nullptr)
                : instanceStart(instanceStart)
                , instanceCount(instanceCount)
                , part(part)
            {
            }
        };

    private:
        Plugin::Core *core = nullptr;
        Video::Device *videoDevice = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;
        Plugin::Editor *editor = nullptr;

        VisualHandle visual;
        Video::BufferPtr instanceBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Model> modelMap;

        std::vector<float, AlignedAllocator<float, 16>> halfSizeXList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeYList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeZList;
        std::vector<float, AlignedAllocator<float, 16>> transformList[16];
        std::vector<bool> visibilityList;

        using InstanceList = concurrency::concurrent_vector<Math::Float4x4>;
        using PartMap = concurrency::concurrent_unordered_map<const Model::Part *, InstanceList>;
        using MaterialMap = concurrency::concurrent_unordered_map<MaterialHandle, PartMap>;
        MaterialMap materialMap;

    public:
        ModelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , videoDevice(core->getVideoDevice())
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
            , loadPool(1)
        {
            assert(core);
            assert(videoDevice);
			assert(population);
            assert(resources);
            assert(renderer);

            LockedWrite{ std::cout } << String::Format("Initializing model system");

            population->onReset.connect(this, &ModelProcessor::onReset);
            population->onEntityCreated.connect(this, &ModelProcessor::onEntityCreated);
            population->onEntityDestroyed.connect(this, &ModelProcessor::onEntityDestroyed);
            population->onComponentAdded.connect(this, &ModelProcessor::onComponentAdded);
            population->onComponentRemoved.connect(this, &ModelProcessor::onComponentRemoved);
            renderer->onQueueDrawCalls.connect(this, &ModelProcessor::onQueueDrawCalls);

            visual = resources->loadVisual("model");

            Video::Buffer::Description instanceDescription;
            instanceDescription.stride = sizeof(Math::Float4x4);
            instanceDescription.count = 100;
            instanceDescription.type = Video::Buffer::Description::Type::Vertex;
            instanceDescription.flags = Video::Buffer::Description::Flags::Mappable;
            instanceBuffer = videoDevice->createBuffer(instanceDescription);
            instanceBuffer->setName("model:instances");
        }

        ~ModelProcessor(void)
        {
        }

        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](bool isNewInsert, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                auto pair = modelMap.insert(std::make_pair(GetHash(modelComponent.name), Model()));
                if (pair.second)
                {
                    LockedWrite{ std::cout } << String::Format("Queueing model for load: %v", modelComponent.name);
                    loadPool.enqueue([this, name = modelComponent.name, &model = pair.first->second](void) -> void
                    {
                        auto fileName(getContext()->getRootFileName("data", "models", name).withExtension(".gek"));
                        if (!fileName.isFile())
                        {
							LockedWrite{ std::cerr } << String::Format("Model file not found: %v", fileName);
                            return;
                        }
                        
                        static const std::vector<uint8_t> EmptyBuffer;
						std::vector<uint8_t> buffer(FileSystem::Load(fileName, EmptyBuffer, sizeof(Header)));
						if (buffer.size() < sizeof(Header))
						{
							LockedWrite{ std::cerr } << String::Format("Model file too small to contain header: %v", fileName);
							return;
						}

                        Header *header = (Header *)buffer.data();
                        if (header->identifier != *(uint32_t *)"GEKX")
                        {
							LockedWrite{ std::cerr } << String::Format("Unknown model file identifier encountered (requires: GEKX, has: %v): %v", header->identifier, fileName);
							return;
                        }

                        if (header->type != 0)
                        {
							LockedWrite{ std::cerr } << String::Format("Unsupported model type encountered (requires: 0, has: %v): %v", header->type, fileName);
							return;
						}

                        if (header->version != 6)
                        {
                            LockedWrite{ std::cerr } << String::Format("Unsupported model version encountered (requires: 6, has: %v): %v", header->version, fileName);
							return;
						}

                        model.boundingBox = header->boundingBox;
                        LockedWrite{ std::cout } << String::Format("Model: %v, %v parts", name, header->partCount);
                        loadPool.enqueue([this, name = name, fileName, &model](void) -> void
                        {
							std::vector<uint8_t> buffer(FileSystem::Load(fileName, EmptyBuffer));

							Header *header = (Header *)buffer.data();
							if (buffer.size() < (sizeof(Header) + (sizeof(Header::Part) * header->partCount)))
							{
								LockedWrite{ std::cerr } << String::Format("Model file too small to contain part headers: %v", fileName);
								return;
							}

                            model.partList.resize(header->partCount);
                            uint8_t *bufferData = (uint8_t *)&header->partList[header->partCount];
                            for (uint32_t partIndex = 0; partIndex < header->partCount; ++partIndex)
                            {
                                Header::Part &partHeader = header->partList[partIndex];
								Model::Part &part = model.partList[partIndex];

                                part.material = resources->loadMaterial(partHeader.name);

                                Video::Buffer::Description indexBufferDescription;
                                indexBufferDescription.format = Video::Format::R16_UINT;
                                indexBufferDescription.count = partHeader.indexCount;
                                indexBufferDescription.type = Video::Buffer::Description::Type::Index;
                                part.indexBuffer = resources->createBuffer(String::Format("model:indices:%v:%v", name, partIndex), indexBufferDescription, reinterpret_cast<uint16_t *>(bufferData));
                                bufferData += (sizeof(uint16_t) * partHeader.indexCount);

                                Video::Buffer::Description vertexBufferDescription;
                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                vertexBufferDescription.count = partHeader.vertexCount;
                                vertexBufferDescription.type = Video::Buffer::Description::Type::Vertex;
                                part.vertexBufferList[0] = resources->createBuffer(String::Format("model:positions:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float2);
                                part.vertexBufferList[1] = resources->createBuffer(String::Format("model:texcoords:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float2 *>(bufferData));
                                bufferData += (sizeof(Math::Float2) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[2] = resources->createBuffer(String::Format("model:tangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[3] = resources->createBuffer(String::Format("model:bitangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                vertexBufferDescription.stride = sizeof(Math::Float3);
                                part.vertexBufferList[4] = resources->createBuffer(String::Format("model:normals:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                part.indexCount = partHeader.indexCount;
                            }
							
							LockedWrite{ std::cout } << String::Format("Model successfully loaded: %v", name);
						});
                    });
                }

                data.model = &pair.first->second;
            });
        }

        // Plugin::Processor
        void onInitialized(void)
        {
            core->listProcessors([&](Plugin::Processor *processor) -> void
            {
                auto castCheck = dynamic_cast<Plugin::Editor *>(processor);
                if (castCheck)
                {
                    (editor = castCheck)->onModified.connect(this, &ModelProcessor::onModified);
                }
            });
        }

        void onDestroyed(void)
        {
            loadPool.drain();
            if (editor)
            {
                editor->onModified.disconnect(this, &ModelProcessor::onModified);
            }

            population->onReset.disconnect(this, &ModelProcessor::onReset);
            population->onEntityCreated.disconnect(this, &ModelProcessor::onEntityCreated);
            population->onEntityDestroyed.disconnect(this, &ModelProcessor::onEntityDestroyed);
            population->onComponentAdded.disconnect(this, &ModelProcessor::onComponentAdded);
            population->onComponentRemoved.disconnect(this, &ModelProcessor::onComponentRemoved);
            renderer->onQueueDrawCalls.disconnect(this, &ModelProcessor::onQueueDrawCalls);
        }

        // Model::Processor
        Shapes::AlignedBox getBoundingBox(std::string const &modelName)
        {
            auto modelSearch = modelMap.find(GetHash(modelName));
            if (modelSearch != std::end(modelMap))
            {
                return modelSearch->second.boundingBox;
            }

            return Shapes::AlignedBox();
        }

        // Plugin::Editor Slots
        void onModified(Plugin::Entity * const entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        // Plugin::Population Slots
        void onReset(void)
        {
            clear();
        }

        void onEntityCreated(Plugin::Entity * const entity)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        // Plugin::Renderer Slots
        void onQueueDrawCalls(const Shapes::Frustum &viewFrustum, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix)
        {
            assert(renderer);

            const auto entityCount = getEntityCount();
            auto buffer = (entityCount % 4);
            buffer = (buffer ? (4 - buffer) : buffer);
            const auto bufferedEntityCount = (entityCount + buffer);
            halfSizeXList.resize(bufferedEntityCount);
            halfSizeYList.resize(bufferedEntityCount);
            halfSizeZList.resize(bufferedEntityCount);
            for (auto &elementList : transformList)
            {
                elementList.resize(bufferedEntityCount);
            }

            size_t entityIndex = 0;
            parallelListEntities([&](Plugin::Entity * const entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                Model const &model = *data.model;
                auto currentEntityIndex = InterlockedIncrement(&entityIndex);

                auto halfSize(model.boundingBox.getSize() * 0.5f);
                halfSizeXList[currentEntityIndex] = halfSize.x * transformComponent.scale.x;
                halfSizeYList[currentEntityIndex] = halfSize.y * transformComponent.scale.y;
                halfSizeZList[currentEntityIndex] = halfSize.z * transformComponent.scale.z;

                auto matrix(transformComponent.getMatrix());
                matrix.translation.xyz += model.boundingBox.getCenter();
                for (size_t element = 0; element < 16; ++element)
                {
                    transformList[element][currentEntityIndex] = matrix.data[element];
                }
            });

            visibilityList.resize(bufferedEntityCount);
            Math::SIMD::cullOrientedBoundingBoxes(viewMatrix, projectionMatrix, bufferedEntityCount, halfSizeXList, halfSizeYList, halfSizeZList, transformList, visibilityList);

            auto entitySearch = std::begin(entityDataMap);
            for (size_t entityIndex = 0; entityIndex < entityCount; ++entityIndex)
            {
                if (visibilityList[entityIndex])
                {
                    auto entity = entitySearch->first;
                    auto &transformComponent = entity->getComponent<Components::Transform>();
                    auto &model = *entitySearch->second.model;
                    ++entitySearch;

                    auto modelViewMatrix(transformComponent.getScaledMatrix() * viewMatrix);
                    concurrency::parallel_for_each(std::begin(model.partList), std::end(model.partList), [&](const Model::Part &part) -> void
                    {
                        auto &partMap = materialMap[part.material];
                        auto &instanceList = partMap[&part];
                        instanceList.push_back(modelViewMatrix);
                    });
                }
            }

			size_t maximumInstanceCount = 0;
            for (auto &materialPair : materialMap)
            {
                const auto material = materialPair.first;
                auto &partMap = materialPair.second;

                size_t partInstanceCount = 0;
                for (const auto &partPair : partMap)
                {
                    const auto part = partPair.first;
                    const auto &partInstanceList = partPair.second;
                    partInstanceCount += partInstanceList.size();
                }

                std::vector<DrawData> drawDataList(partMap.size());
                std::vector<Math::Float4x4> instanceList(partInstanceCount);
                for (auto &partPair : partMap)
                {
                    auto part = partPair.first;
                    if (part)
                    {
                        auto &partInstanceList = partPair.second;
                        drawDataList.push_back(DrawData(instanceList.size(), partInstanceList.size(), part));
                        instanceList.insert(std::end(instanceList), std::begin(partInstanceList), std::end(partInstanceList));
                        partInstanceList.clear();
                    }
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
                        for (const auto &drawData : drawDataList)
                        {
                            if (drawData.part)
                            {
                                resources->setVertexBufferList(videoContext, drawData.part->vertexBufferList, 0);
                                resources->setIndexBuffer(videoContext, drawData.part->indexBuffer, 0);
                                resources->drawInstancedIndexedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, drawData.part->indexCount, 0, 0);
                            }
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
                instanceBuffer->setName("model:instances");
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

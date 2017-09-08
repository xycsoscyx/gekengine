#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/SIMD.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
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
    private:
        int selectedModel = 0;
        std::vector<std::string> modelList;
        std::string const modelsPath;

    public:
        Model(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
            , modelsPath(context->getRootFileName("data", "models").string())
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
                if (modelList.empty())
                {
                    FileSystem::Find(modelsPath, [&](FileSystem::Path const &filePath) -> bool
                    {
                        if (filePath.isDirectory())
                        {
                            modelList.push_back(filePath.getFileName());
                        }

                        return true;
                    });
                }

                if (modelComponent.name.empty())
                {
                    selectedModel = 0;
                }
                else if (selectedModel <= 0 || selectedModel >= modelList.size() || modelList[selectedModel - 1] != modelComponent.name)
                {
                    auto modelSearch = std::find_if(std::begin(modelList), std::end(modelList), [&](std::string const &modelName) -> bool
                    {
                        return (modelName == modelComponent.name);
                    });

                    if (modelSearch != std::end(modelList))
                    {
                        selectedModel = (std::distance(std::begin(modelList), modelSearch) + 1);
                    }
                    else
                    {
                        selectedModel = 0;
                    }
                }

                return ImGui::Combo("##model", &selectedModel, [](void *userData, int index, char const **outputText) -> bool
                {
                    if (index == 0)
                    {
                        *outputText = "(none)";
                        return true;
                    }

                    auto &modelList = *(std::vector<std::string> *)userData;
                    if (index > 0 && index <= modelList.size())
                    {
                        *outputText = modelList[index - 1].c_str();
                        return true;
                    }

                    return false;
                }, &modelList, (modelList.size() + 1), 10);
            });

            if (changed)
            {
                if (selectedModel == 0)
                {
                    modelComponent.name.clear();
                }
                else
                {
                    modelComponent.name = modelList[selectedModel - 1];
                }
            }

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
            struct Material
            {
                char name[64];
                struct Level
                {
                    uint32_t vertexCount = 0;
                    uint32_t indexCount = 0;
                };
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;
            uint8_t levelCount = 0;

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

        struct Model
        {
            struct Material
            {
                MaterialHandle handle;
                struct Level
                {
                    std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(5);
                    ResourceHandle indexBuffer;
                    uint32_t indexCount = 0;
                };

                std::vector<Level> levelList;
            };

            struct Mesh
            {
                Shapes::AlignedBox boundingBox;
                std::vector<Material> materialList;
            };

            std::vector<Mesh> meshList;
            Shapes::AlignedBox boundingBox;
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
            const Model::Material::Level *data = nullptr;

            DrawData(uint32_t instanceStart = 0, uint32_t instanceCount = 0, const Model::Material::Level *data = nullptr)
                : instanceStart(instanceStart)
                , instanceCount(instanceCount)
                , data(data)
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
        using MaterialMap = concurrency::concurrent_unordered_map<const Model::Material *, InstanceList>;
        using HandleMap = concurrency::concurrent_unordered_map<MaterialHandle, MaterialMap>;
        HandleMap renderList;

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

            core->onInitialized.connect(this, &ModelProcessor::onInitialized);
            core->onShutdown.connect(this, &ModelProcessor::onShutdown);
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
            instanceDescription.type = Video::Buffer::Type::Vertex;
            instanceDescription.flags = Video::Buffer::Flags::Mappable;
            instanceBuffer = videoDevice->createBuffer(instanceDescription);
            instanceBuffer->setName("model:instances");
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
                        std::vector<FileSystem::Path> meshPathList;
                        auto modelPath(getContext()->getRootFileName("data", "models", name));
                        FileSystem::Find(modelPath, [&](FileSystem::Path const &filePath) -> bool
                        {
                            std::string fileName(filePath.u8string());
                            if (filePath.isFile() && String::GetLower(filePath.getExtension()) == ".gek")
                            {
                                static const std::vector<uint8_t> EmptyBuffer;
                                std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer, sizeof(Header)));
                                if (buffer.size() < sizeof(Header))
                                {
                                    LockedWrite{ std::cerr } << String::Format("Model file too small to contain header: %v", fileName);
                                    return true;
                                }

                                Header *header = (Header *)buffer.data();
                                if (header->identifier != *(uint32_t *)"GEKX")
                                {
                                    LockedWrite{ std::cerr } << String::Format("Unknown model file identifier encountered (requires: GEKX, has: %v): %v", header->identifier, fileName);
                                    return true;
                                }

                                if (header->type != 0)
                                {
                                    LockedWrite{ std::cerr } << String::Format("Unsupported model type encountered (requires: 0, has: %v): %v", header->type, fileName);
                                    return true;
                                }

                                if (header->version != 7)
                                {
                                    LockedWrite{ std::cerr } << String::Format("Unsupported model version encountered (requires: 7, has: %v): %v", header->version, fileName);
                                    return true;
                                }

                                meshPathList.push_back(filePath);
                            }

                            return true;
                        });

                        model.meshList.resize(meshPathList.size());
                        for (size_t meshIndex = 0; meshIndex < meshPathList.size(); ++meshIndex)
                        {
                            auto &mesh = model.meshList[meshIndex];
                            auto &filePath = meshPathList[meshIndex];
                            loadPool.enqueue([this, name = name, filePath, &model, &mesh](void) -> void
                            {
                                auto fileName(filePath.getFileName());

                                static const std::vector<uint8_t> EmptyBuffer;
                                std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer));

                                Header *header = (Header *)buffer.data();
                                if (buffer.size() < (sizeof(Header) + (sizeof(Header::Material) * header->materialCount)))
                                {
                                    LockedWrite{ std::cerr } << String::Format("Model file too small to contain material headers: %v", filePath.u8string());
                                    return;
                                }

                                LockedWrite{ std::cout } << String::Format("Model %v, Mesh %v: %v materials", name, fileName, header->materialCount);

                                mesh.boundingBox = header->boundingBox;
                                model.boundingBox.extend(mesh.boundingBox.minimum);
                                model.boundingBox.extend(mesh.boundingBox.maximum);
                                mesh.materialList.resize(header->materialCount);
                                uint8_t *bufferData = (uint8_t *)&header->materialList[header->materialCount];
                                for (uint32_t materialIndex = 0; materialIndex < header->materialCount; ++materialIndex)
                                {
                                    Header::Material &materialHeader = header->materialList[materialIndex];
                                    Model::Material &material = mesh.materialList[materialIndex];

                                    material.handle = resources->loadMaterial(materialHeader.name);

                                    material.levelList.resize(header->levelCount);
                                    for (uint8_t levelIndex = 0; levelIndex < header->levelCount; ++levelIndex)
                                    {
                                        auto &level = material.levelList[levelIndex];

                                        auto &levelHeader = *(Header::Material::Level *)bufferData;
                                        bufferData += sizeof(Header::Material::Level);

                                        Video::Buffer::Description indexBufferDescription;
                                        indexBufferDescription.format = Video::Format::R16_UINT;
                                        indexBufferDescription.count = levelHeader.indexCount;
                                        indexBufferDescription.type = Video::Buffer::Type::Index;
                                        level.indexBuffer = resources->createBuffer(String::Format("model:%v:%v:indices:%v", name, fileName, materialIndex), indexBufferDescription, reinterpret_cast<uint16_t *>(bufferData));
                                        bufferData += (sizeof(uint16_t) * levelHeader.indexCount);

                                        Video::Buffer::Description vertexBufferDescription;
                                        vertexBufferDescription.stride = sizeof(Math::Float3);
                                        vertexBufferDescription.count = levelHeader.vertexCount;
                                        vertexBufferDescription.type = Video::Buffer::Type::Vertex;
                                        level.vertexBufferList[0] = resources->createBuffer(String::Format("model:%v:%v:positions:%v", name, fileName, materialIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                        bufferData += (sizeof(Math::Float3) * levelHeader.vertexCount);

                                        vertexBufferDescription.stride = sizeof(Math::Float2);
                                        level.vertexBufferList[1] = resources->createBuffer(String::Format("model:%v:%v:texcoords:%v", name, fileName, materialIndex), vertexBufferDescription, reinterpret_cast<Math::Float2 *>(bufferData));
                                        bufferData += (sizeof(Math::Float2) * levelHeader.vertexCount);

                                        vertexBufferDescription.stride = sizeof(Math::Float3);
                                        level.vertexBufferList[2] = resources->createBuffer(String::Format("model:%v:%v:tangents:%v", name, fileName, materialIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                        bufferData += (sizeof(Math::Float3) * levelHeader.vertexCount);

                                        vertexBufferDescription.stride = sizeof(Math::Float3);
                                        level.vertexBufferList[3] = resources->createBuffer(String::Format("model:%v:%v:bitangents:%v", name, fileName, materialIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                        bufferData += (sizeof(Math::Float3) * levelHeader.vertexCount);

                                        vertexBufferDescription.stride = sizeof(Math::Float3);
                                        level.vertexBufferList[4] = resources->createBuffer(String::Format("model:%v:%v:normals:%v", name, fileName, materialIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                        bufferData += (sizeof(Math::Float3) * levelHeader.vertexCount);

                                        level.indexCount = levelHeader.indexCount;
                                    }
                                }

                                LockedWrite{ std::cout } << String::Format("Model %v, mesh successfully loaded: %v", name, fileName);
                            });
                        }

                        LockedWrite{ std::cout } << String::Format("Model successfully loaded: %v", name);
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

        void onShutdown(void)
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
            if (type == typeid(Components::Model))
            {
                addEntity(entity);
            }
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
                    concurrency::parallel_for_each(std::begin(model.meshList), std::end(model.meshList), [&](const Model::Mesh &mesh) -> void
                    {
                        concurrency::parallel_for_each(std::begin(mesh.materialList), std::end(mesh.materialList), [&](const Model::Material &material) -> void
                        {
                            auto &materialMap = renderList[material.handle];
                            auto &instanceList = materialMap[&material];
                            instanceList.push_back(modelViewMatrix);
                        });
                    });
                }
            }

			size_t maximumInstanceCount = 0;
            for (auto &materialPair : renderList)
            {
                const auto material = materialPair.first;
                auto &materialMap = materialPair.second;

                size_t materialInstanceCount = 0;
                for (auto const &materialPair : materialMap)
                {
                    const auto material = materialPair.first;
                    auto const &materialInstanceList = materialPair.second;
                    materialInstanceCount += materialInstanceList.size();
                }

                std::vector<DrawData> drawDataList(materialMap.size());
                std::vector<Math::Float4x4> instanceList(materialInstanceCount);
                for (auto &materialPair : materialMap)
                {
                    auto material = materialPair.first;
                    if (material)
                    {
                        auto &materialInstanceList = materialPair.second;
                        drawDataList.push_back(DrawData(instanceList.size(), materialInstanceList.size(), &material->levelList[1]));
                        instanceList.insert(std::end(instanceList), std::begin(materialInstanceList), std::end(materialInstanceList));
                        materialInstanceList.clear();
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
                        for (auto const &drawData : drawDataList)
                        {
                            if (drawData.data)
                            {
                                auto &level = *drawData.data;
                                resources->setVertexBufferList(videoContext, level.vertexBufferList, 0);
                                resources->setIndexBuffer(videoContext, level.indexBuffer, 0);
                                resources->drawInstancedIndexedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, level.indexCount, 0, 0);
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
                instanceDescription.type = Video::Buffer::Type::Vertex;
                instanceDescription.flags = Video::Buffer::Flags::Mappable;
                instanceBuffer = videoDevice->createBuffer(instanceDescription);
                instanceBuffer->setName("model:instances");
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

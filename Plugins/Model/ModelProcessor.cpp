#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/SIMD.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Allocator.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/RenderDevice.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Visualizer.hpp"
#include "GEK/API/Resources.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Model/Base.hpp"
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>
#include <xmmintrin.h>
#include <algorithm>
#include <execution>
#include <memory>
#include <future>
#include <mutex>
#include <unordered_set>

namespace Gek
{
    #include "Cube.h"
    #include "Sphere.h"

    template<typename TYPE>
    void updateMaximumValue(std::atomic<TYPE>& maximum_value, TYPE const& value) noexcept
    {
        TYPE prev_value = maximum_value;
        while (prev_value < value && !maximum_value.compare_exchange_weak(prev_value, value))
        {
        }
    }

    class Unpacker
    {
    private:
        uint8_t *buffer;

    public:
        Unpacker(uint8_t *buffer)
            : buffer(buffer)
        {
        }

        template <typename TYPE>
        TYPE const *readBlock(size_t count)
        {
            TYPE *data = reinterpret_cast<TYPE *>(buffer);
            buffer += (sizeof(TYPE) * count);
            return data;
        }
    };

    GEK_CONTEXT_USER(Model, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Model, Edit::Component>
    {
    private:
        int selectedModel = 0;
        std::vector<std::string> modelList;

    public:
        Model(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Model const * const data, JSON::Object &exportData) const
        {
            exportData = data->name;
        }

        void load(Components::Model * const data, JSON::Object const &importData)
        {
            data->name = evaluate(importData, String::Empty);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &modelComponent = *dynamic_cast<Components::Model *>(data);

            std::function<FileSystem::Path(const char*, FileSystem::Path const&)> removeRoot = [](const char* location, FileSystem::Path const& path) -> FileSystem::Path
            {
                auto parentPath = path.getParentPath();
                while (parentPath.isDirectory() && parentPath != path.getRootPath())
                {
                    if (parentPath.getFileName() == location)
                    {
                        return path.lexicallyRelative(parentPath);
                    }
                    else
                    {
                        parentPath = parentPath.getParentPath();
                    }
                };

                return path;
            };

            changed |= editorElement("Model", [&](void) -> bool
            {
                if (modelList.empty())
                {
                    std::function<bool(FileSystem::Path const &)> searchDirectory;
                    searchDirectory = ([&](FileSystem::Path const &filePath) -> bool
                    {
                        if (filePath.isDirectory())
                        {
                            filePath.findFiles(searchDirectory);
                        }
                        else if (filePath.getExtension() == ".gek")
                        {
                            auto modelPath = removeRoot("models", filePath).getParentPath().getString();
                            modelList.push_back(modelPath);
                            return false;
                        }

                        return true;
                    });

                    getContext()->findDataFiles("models", searchDirectory);
                }

                if (modelComponent.name.empty())
                {
                    selectedModel = 0;
                }
                else
                {
                    constexpr int FirstFileModelIndex = 3;
                    const int fileModelCount = static_cast<int>(modelList.size());
                    const bool hasValidFileSelection =
                        (selectedModel >= FirstFileModelIndex) &&
                        (selectedModel < (FirstFileModelIndex + fileModelCount));

                    if (modelComponent.name == "#cube")
                    {
                        selectedModel = 1;
                    }
                    else if (modelComponent.name == "#sphere")
                    {
                        selectedModel = 2;
                    }
                    else if (!hasValidFileSelection || modelList[selectedModel - FirstFileModelIndex] != modelComponent.name)
                    {
                        auto modelSearch = std::find_if(std::begin(modelList), std::end(modelList), [&](std::string const& modelName) -> bool
                        {
                            return (modelName == modelComponent.name);
                        });

                        if (modelSearch != std::end(modelList))
                        {
                            selectedModel = static_cast<int>(std::distance(std::begin(modelList), modelSearch)) + FirstFileModelIndex;
                        }
                        else
                        {
                            selectedModel = 0;
                        }
                    }
                }

                constexpr int FirstFileModelIndex = 3;
                const int itemCount = static_cast<int>(modelList.size()) + FirstFileModelIndex;
                auto getModelLabel = [&](int index) -> const char *
                {
                    switch (index)
                    {
                    case 0:
                        return "(none)";
                    case 1:
                        return "* Physics Cube";
                    case 2:
                        return "* Physics Sphere";
                    default:
                        break;
                    }

                    const int modelIndex = index - FirstFileModelIndex;
                    if (modelIndex < 0 || modelIndex >= static_cast<int>(modelList.size()))
                    {
                        return "";
                    }

                    return modelList[modelIndex].c_str();
                };

                if (selectedModel < 0 || selectedModel >= itemCount)
                {
                    selectedModel = 0;
                }

                bool selectionChanged = false;
                if (ImGui::BeginCombo("##model", getModelLabel(selectedModel)))
                {
                    for (int index = 0; index < itemCount; ++index)
                    {
                        const bool isSelected = (selectedModel == index);
                        if (ImGui::Selectable(getModelLabel(index), isSelected))
                        {
                            selectedModel = index;
                            selectionChanged = true;
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                return selectionChanged;
            });

            if (changed)
            {
                switch (selectedModel)
                {
                case 0:
                    modelComponent.name.clear();
                    break;

                case 1:
                    modelComponent.name = "#cube";
                    break;

                case 2:
                    modelComponent.name = "#sphere";
                    break;

                default:
                    modelComponent.name = modelList[selectedModel - 3];
                    break;
                };
            }

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::EntityProcessor<ModelProcessor, Components::Model, Components::Transform>
        , public Gek::Processor::Model
    {
    public:
        struct Header
        {
            struct Mesh
            {
                char material[64];
                uint32_t vertexCount = 0;
                uint32_t faceCount = 0;
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;

            Shapes::AlignedBox boundingBox;

            uint32_t meshCount = 0;
            Mesh meshList[1];
        };

        struct Face
        {
            uint16_t data[3];
            uint16_t &operator [] (size_t index)
            {
                return data[index];
            }
        };

        struct Group
        {
            struct Model
            {
                struct Mesh
                {
                    MaterialHandle material;
                    std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(4);
                    ResourceHandle indexBuffer;
                    uint32_t indexCount = 0;
                    uint32_t vertexCount = 0;
                };

                Shapes::AlignedBox boundingBox;
                std::vector<Mesh> meshList;
            };

            std::vector<Model> modelList;
            Shapes::AlignedBox boundingBox;
            std::atomic_bool ready = false;
        };

        struct Data
        {
            std::shared_ptr<Group> group;
        };

        struct DrawData
        {
            uint32_t instanceStart = 0;
            uint32_t instanceCount = 0;
            const Group::Model::Mesh *data = nullptr;

            DrawData(uint32_t instanceStart = 0, uint32_t instanceCount = 0, Group::Model::Mesh const *data = nullptr)
                : instanceStart(instanceStart)
                , instanceCount(instanceCount)
                , data(data)
            {
            }
        };

    private:
        Plugin::Core *core = nullptr;
        Render::Device *videoDevice = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Visualizer *renderer = nullptr;
        Edit::Events *events = nullptr;

        VisualHandle visual;
        Render::BufferPtr instanceBuffer;
        ThreadPool loadPool;

        tbb::concurrent_unordered_map<std::size_t, std::shared_ptr<Group>> groupMap;

        std::vector<float, AlignedAllocator<float, 16>> halfSizeXList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeYList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeZList;
        std::vector<float, AlignedAllocator<float, 16>> transformList[16];
        std::vector<bool> visibilityList;

        using EntityDataList = tbb::concurrent_vector<std::tuple<Plugin::Entity * const, Data const *, uint32_t>>;
        using EntityModelList = tbb::concurrent_vector<std::tuple<Plugin::Entity * const, Group::Model const *, uint32_t>>;
        EntityDataList entityDataList;
        EntityModelList entityModelList;

        using InstanceList = tbb::concurrent_vector<Math::Float4x4>;
        using MeshInstanceMap = tbb::concurrent_unordered_map<const Group::Model::Mesh *, InstanceList>;
        using MaterialMeshMap = tbb::concurrent_unordered_map<MaterialHandle, MeshInstanceMap>;
        MaterialMeshMap renderList;

        bool shuttingDown = false;
        std::mutex missingMaterialMutex;
        std::unordered_set<std::string> warnedMissingMaterials;

    public:
        ModelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , videoDevice(core->getVisualizer()->getRenderDevice())
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getVisualizer())
            , loadPool(5)
        {
            assert(core);
            assert(videoDevice);
			assert(population);
            assert(resources);
            assert(renderer);

            getContext()->log(Context::Info, "Initializing model system");

            core->onInitialized.connect(this, &ModelProcessor::onInitialized);
            core->onShutdown.connect(this, &ModelProcessor::onShutdown);
            population->onReset.connect(this, &ModelProcessor::onReset);
            population->onEntityCreated.connect(this, &ModelProcessor::onEntityCreated);
            population->onEntityDestroyed.connect(this, &ModelProcessor::onEntityDestroyed);
            population->onComponentAdded.connect(this, &ModelProcessor::onComponentAdded);
            population->onComponentRemoved.connect(this, &ModelProcessor::onComponentRemoved);
            renderer->onQueueDrawCalls.connect(this, &ModelProcessor::onQueueDrawCalls);

            visual = resources->loadVisual("model");

            Render::Buffer::Description instanceDescription;
            instanceDescription.name = "model:instances";
            instanceDescription.stride = sizeof(Math::Float4x4);
            instanceDescription.count = 100;
            instanceDescription.type = Render::Buffer::Type::Vertex;
            instanceDescription.flags = Render::Buffer::Flags::Mappable;
            instanceBuffer = videoDevice->createBuffer(instanceDescription);
        }

        void scheduleLoadMesh(Header::Mesh& meshHeader, Group::Model::Mesh& mesh, uint32_t meshIndex, std::string fileName, std::string name, uint8_t* meshBuffer, std::shared_ptr<std::vector<uint8_t>> buffer)
        {
            if (shuttingDown)
            {
                return;
            }

            Unpacker unpacker(meshBuffer);
            mesh.material = resources->loadMaterial(meshHeader.material);
            if (!mesh.material)
            {
                auto const missingMaterialName = std::string(meshHeader.material);
                bool shouldLog = false;
                {
                    std::lock_guard<std::mutex> lock(missingMaterialMutex);
                    shouldLog = warnedMissingMaterials.insert(missingMaterialName).second;
                }

                if (shouldLog)
                {
                    getContext()->log(Context::Warning,
                        "Unable to resolve material '{}' while loading model '{}' in group '{}'; using debug fallback material",
                        missingMaterialName,
                        fileName,
                        name);
                }

                mesh.material = resources->loadMaterial("debug");
            }
            mesh.vertexCount = meshHeader.vertexCount;
            mesh.indexCount = (meshHeader.faceCount * 3);

            //Render::Buffer::Description indexBufferDescription;
            //indexBufferDescription.format = Render::Format::R16_UINT;
            //indexBufferDescription.count = (meshHeader.faceCount * 3);
            //indexBufferDescription.type = Render::Buffer::Type::Index;
            //mesh.indexBuffer = resources->createBuffer(std::format("model:{}.{}.{}:indices", meshIndex, fileName, name), indexBufferDescription, unpacker.readBlock<Face>(meshHeader.faceCount));

            Render::Buffer::Description vertexBufferDescription;
            vertexBufferDescription.name = std::format("model:{}.{}.{}:positions", meshIndex, fileName, name);
            vertexBufferDescription.stride = sizeof(Math::Float3);
            vertexBufferDescription.count = meshHeader.vertexCount;
            vertexBufferDescription.type = Render::Buffer::Type::Vertex;
            mesh.vertexBufferList[0] = resources->createBuffer(vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));

            vertexBufferDescription.stride = sizeof(Math::Float2);
            vertexBufferDescription.name = std::format("model:{}.{}.{}:texcoords", meshIndex, fileName, name);
            mesh.vertexBufferList[1] = resources->createBuffer(vertexBufferDescription, unpacker.readBlock<Math::Float2>(meshHeader.vertexCount));

            vertexBufferDescription.stride = sizeof(Math::Float4);
            vertexBufferDescription.name = std::format("model:{}.{}.{}:tangents", meshIndex, fileName, name);
            mesh.vertexBufferList[2] = resources->createBuffer(vertexBufferDescription, unpacker.readBlock<Math::Float4>(meshHeader.vertexCount));

            vertexBufferDescription.stride = sizeof(Math::Float3);
            vertexBufferDescription.name = std::format("model:{}.{}.{}:normals", meshIndex, fileName, name);
            mesh.vertexBufferList[3] = resources->createBuffer(vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));
        }

        void scheduleLoadData(std::string name, FileSystem::Path filePath, ModelProcessor::Group &group, ModelProcessor::Group::Model &model)
        {
            if (shuttingDown)
            {
                return;
            }

            auto fileName(filePath.getFileName());

            std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(FileSystem::Load(filePath));

            Header* header = (Header*)buffer->data();
            if (buffer->size() < (sizeof(Header) + (sizeof(Header::Mesh) * header->meshCount)))
            {
                getContext()->log(Context::Error, "Model file too small to contain mesh headers: {}", filePath.getString());
                return;
            }

            getContext()->log(Context::Info, "Group {}, loading model {}: {} meshes", name, fileName, header->meshCount);

            model.boundingBox = header->boundingBox;
            group.boundingBox.extend(model.boundingBox.minimum);
            group.boundingBox.extend(model.boundingBox.maximum);
            model.meshList.resize(header->meshCount);
            uint8_t *meshBuffer = (uint8_t*)&header->meshList[header->meshCount];
            for (uint32_t meshIndex = 0; meshIndex < header->meshCount; ++meshIndex)
            {
                Header::Mesh& meshHeader = header->meshList[meshIndex];
                Group::Model::Mesh& mesh = model.meshList[meshIndex];

                scheduleLoadMesh(meshHeader, mesh, meshIndex, fileName, name, meshBuffer, buffer);
                meshBuffer += (meshHeader.vertexCount * (sizeof(Math::Float3) + sizeof(Math::Float2) + sizeof(Math::Float4) + sizeof(Math::Float3)));
            }

            getContext()->log(Context::Info, "Group {}, mesh {} successfully loaded", name, fileName);
        }

        Task scheduleLoadGroup(std::string name, std::shared_ptr<ModelProcessor::Group> group)
        {
            getContext()->log(Context::Info, "Queueing group for load: {}", name);

            co_await loadPool.schedule();
            if (shuttingDown || !group)
            {
                co_return;
            }

            Group loadedGroup;

            if (name == "#cube")
            {
                loadedGroup.modelList.resize(1);
                auto& model = loadedGroup.modelList[0];
                for (auto& staticModel : cube_models)
                {
                    auto& mesh = model.meshList.emplace_back();
                    mesh.material = resources->loadMaterial(staticModel.material);
                    mesh.vertexCount = static_cast<uint32_t>(staticModel.positions.size());

                    Render::Buffer::Description vertexBufferDescription;
                    vertexBufferDescription.name = std::format("model:cube.{}:positions", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float3);
                    vertexBufferDescription.count = static_cast<uint32_t>(staticModel.positions.size());
                    vertexBufferDescription.type = Render::Buffer::Type::Vertex;
                    mesh.vertexBufferList[0] = resources->createBuffer(vertexBufferDescription, staticModel.positions.data());
                    for (auto& position : staticModel.positions)
                    {
                        loadedGroup.boundingBox.extend(position);
                        model.boundingBox.extend(position);
                    }

                    loadedGroup.boundingBox.extend(model.boundingBox.minimum);
                    loadedGroup.boundingBox.extend(model.boundingBox.maximum);
                    vertexBufferDescription.name = std::format("model:cube.{}:texCoords", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float2);
                    mesh.vertexBufferList[1] = resources->createBuffer(vertexBufferDescription, staticModel.texCoords.data());

                    vertexBufferDescription.name = std::format("model:cube.{}:tangents", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float4);
                    mesh.vertexBufferList[2] = resources->createBuffer(vertexBufferDescription, staticModel.tangents.data());

                    vertexBufferDescription.name = std::format("model:cube.{}:normals", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float3);
                    mesh.vertexBufferList[3] = resources->createBuffer(vertexBufferDescription, staticModel.normals.data());
                }
            }
            else if (name == "#sphere")
            {
                loadedGroup.modelList.resize(1);
                auto& model = loadedGroup.modelList[0];
                for (auto& staticModel : sphere_models)
                {
                    auto& mesh = model.meshList.emplace_back();
                    mesh.material = resources->loadMaterial(staticModel.material);
                    mesh.vertexCount = static_cast<uint32_t>(staticModel.positions.size());

                    Render::Buffer::Description vertexBufferDescription;
                    vertexBufferDescription.name = std::format("model:sphere.{}:positions", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float3);
                    vertexBufferDescription.count = static_cast<uint32_t>(staticModel.positions.size());
                    vertexBufferDescription.type = Render::Buffer::Type::Vertex;
                    mesh.vertexBufferList[0] = resources->createBuffer(vertexBufferDescription, staticModel.positions.data());
                    for (auto& position : staticModel.positions)
                    {
                        loadedGroup.boundingBox.extend(position);
                        model.boundingBox.extend(position);
                    }

                    loadedGroup.boundingBox.extend(model.boundingBox.minimum);
                    loadedGroup.boundingBox.extend(model.boundingBox.maximum);
                    vertexBufferDescription.name = std::format("model:sphere.{}:texCoords", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float2);
                    mesh.vertexBufferList[1] = resources->createBuffer(vertexBufferDescription, staticModel.texCoords.data());

                    vertexBufferDescription.name = std::format("model:sphere.{}:tangents", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float4);
                    mesh.vertexBufferList[2] = resources->createBuffer(vertexBufferDescription, staticModel.tangents.data());

                    vertexBufferDescription.name = std::format("model:sphere.{}:normals", model.meshList.size());
                    vertexBufferDescription.stride = sizeof(Math::Float3);
                    mesh.vertexBufferList[3] = resources->createBuffer(vertexBufferDescription, staticModel.normals.data());
                }
            }
            else
            {
                std::vector<FileSystem::Path> modelPathList;
                auto groupPath(getContext()->findDataPath(FileSystem::CreatePath("models", name)));
                groupPath.findFiles([&](FileSystem::Path const& filePath) -> bool
                {
                    std::string fileName(filePath.getString());
                    if (filePath.isFile() && String::GetLower(filePath.getExtension()) == ".gek")
                    {
                        std::vector<uint8_t> buffer(FileSystem::Load(filePath, sizeof(Header)));
                        if (buffer.size() < sizeof(Header))
                        {
                            getContext()->log(Context::Error, "Model file too small to contain header: {}", fileName);
                            return true;
                        }

                        Header* header = (Header*)buffer.data();
                        if (header->identifier != *(uint32_t*)"GEKX")
                        {
                            getContext()->log(Context::Error, "Unknown model file identifier encountered (requires: GEKX, has: {}): {}", header->identifier, fileName);
                            return true;
                        }

                        if (header->type != 0)
                        {
                            getContext()->log(Context::Error, "Unsupported model type encountered (requires: 0, has: {}): {}", header->type, fileName);
                            return true;
                        }

                        if (header->version != 8)
                        {
                            getContext()->log(Context::Error, "Unsupported model version encountered (requires: 8, has {}): {}", header->version, fileName);
                            return true;
                        }

                        modelPathList.push_back(filePath);
                    }

                    return true;
                });

                if (modelPathList.empty())
                {
                    getContext()->log(Context::Error, "No models found for group: {}", name);
                }

                loadedGroup.modelList.resize(modelPathList.size());
                for (size_t modelIndex = 0; modelIndex < modelPathList.size(); ++modelIndex)
                {
                    auto& model = loadedGroup.modelList[modelIndex];
                    auto& filePath = modelPathList[modelIndex];
                    scheduleLoadData(name, filePath, loadedGroup, model);
                }
            }

            group->boundingBox = loadedGroup.boundingBox;
            group->modelList = std::move(loadedGroup.modelList);
            group->ready.store(true, std::memory_order_release);

            getContext()->log(Context::Info, "Group {} successfully queued", name);
        }

        void addEntity(Plugin::Entity * const entity)
        {
            EntityProcessor::addEntity(entity, [&](bool isNewInsert, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                if (modelComponent.name.empty())
                {
                    data.group = nullptr;
                }
                else
                {
                    auto pair = groupMap.insert(std::make_pair(GetHash(modelComponent.name), std::make_shared<Group>()));
                    if (pair.second)
                    {
                        pair.first->second->ready.store(false, std::memory_order_relaxed);
                        scheduleLoadGroup(modelComponent.name, pair.first->second);
                    }

                    data.group = pair.first->second;
                }
            });
        }

        // Plugin::Processor
        void onInitialized(void)
        {
            core->listProcessors([&](Plugin::Processor *processor) -> void
            {
                auto castCheck = dynamic_cast<Edit::Events *>(processor);
                if (castCheck)
                {
                    (events = castCheck)->onModified.connect(this, &ModelProcessor::onModified);
                }
            });
        }

        void onShutdown(void)
        {
            shuttingDown = true;

            loadPool.drain();
            if (events)
            {
                events->onModified.disconnect(this, &ModelProcessor::onModified);
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
            auto modelSearch = groupMap.find(GetHash(modelName));
            if ((modelSearch != std::end(groupMap)) && modelSearch->second && modelSearch->second->ready.load(std::memory_order_acquire))
            {
                return modelSearch->second->boundingBox;
            }

            return Shapes::AlignedBox();
        }

        // Plugin::Editor Slots
        void onModified(Plugin::Entity * const entity, Hash type)
        {
            if (type == Components::Model::GetIdentifier())
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

        // Plugin::Visualizer Slots
        void onQueueDrawCalls(Shapes::Frustum const &viewFrustum, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix)
        {
            assert(renderer);
            static uint64_t modelQueueFrameCounter = 0;
            ++modelQueueFrameCounter;

			// Cull by entity/group
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

			entityDataList.clear();
			entityDataList.reserve(static_cast<EntityDataList::size_type>(entityCount));
			parallelListEntities([&](Plugin::Entity * const entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
			{
                if (data.group && data.group->ready.load(std::memory_order_acquire))
                {
                    auto group = data.group;
                    auto matrix(transformComponent.getMatrix());
                    matrix.translation() += group->boundingBox.getCenter();
                    auto halfSize(group->boundingBox.getHalfSize() * transformComponent.scale);

                    auto entityInsert = entityDataList.push_back(std::make_tuple(entity, &data, 0));
					auto entityIndex = std::get<2>(*entityInsert) = static_cast<uint32_t>(std::distance(std::begin(entityDataList), entityInsert));

                    halfSizeXList[entityIndex] = halfSize.x;
                    halfSizeYList[entityIndex] = halfSize.y;
                    halfSizeZList[entityIndex] = halfSize.z;
                    for (size_t element = 0; element < 16; ++element)
                    {
                        transformList[element][entityIndex] = matrix.data[element];
                    }
                }
			});

			visibilityList.resize(bufferedEntityCount);
            Math::SIMD::cullOrientedBoundingBoxes(viewMatrix, projectionMatrix, static_cast<uint32_t>(bufferedEntityCount), halfSizeXList, halfSizeYList, halfSizeZList, transformList, visibilityList);

            const auto visibleEntityCount = std::accumulate(std::begin(entityDataList), std::end(entityDataList), size_t(0), [&](size_t count, auto const &entitySearch) -> size_t
            {
                return (count + (visibilityList[std::get<2>(entitySearch)] ? 1 : 0));
            });

            if (!entityDataList.empty() && visibleEntityCount == 0)
            {
                for (auto const &entitySearch : entityDataList)
                {
                    visibilityList[std::get<2>(entitySearch)] = true;
                }

                if ((modelQueueFrameCounter % 240) == 0)
                {
                    getContext()->log(Context::Warning, "ModelProcessor culling fallback: forcing entity visibility (entities={})", entityDataList.size());
                }
            }

			// Cull by model inside group
            const auto modelCount = std::accumulate(std::begin(entityDataList), std::end(entityDataList), size_t(0), [this](size_t count, auto const &entitySearch) -> size_t
			{
				if (visibilityList[std::get<2>(entitySearch)])
				{
					auto data = std::get<1>(entitySearch);
					count += data->group->modelList.size();
				}

				return count;
			});

			buffer = (modelCount % 4);
			buffer = (buffer ? (4 - buffer) : buffer);
			const auto bufferedModelCount = (modelCount + buffer);
			halfSizeXList.resize(bufferedModelCount);
			halfSizeYList.resize(bufferedModelCount);
			halfSizeZList.resize(bufferedModelCount);
			for (auto &elementList : transformList)
			{
				elementList.resize(bufferedModelCount);
			}

			entityModelList.clear();
            entityModelList.reserve(static_cast<EntityModelList::size_type>(bufferedModelCount));
			std::for_each(std::execution::par, std::begin(entityDataList), std::end(entityDataList), [&](auto &entitySearch) -> void
			{
				auto entityDataIndex = std::get<2>(entitySearch);
				if (visibilityList[entityDataIndex])
				{
					Plugin::Entity * const entity = std::get<0>(entitySearch);
					auto data = std::get<1>(entitySearch);
					auto group = data->group;

					auto &transformComponent = entity->getComponent<Components::Transform>();
					auto matrix(transformComponent.getMatrix());

					std::for_each(std::execution::par, std::begin(group->modelList), std::end(group->modelList), [&](Group::Model const &model) -> void
					{
                        auto halfSize(model.boundingBox.getHalfSize() * transformComponent.scale);
                        auto center(model.boundingBox.getCenter() * transformComponent.scale);
                        auto centerTransform(matrix);
                        centerTransform.translation() = matrix.transform(center);

						auto entityInsert = entityModelList.push_back(std::make_tuple(entity, &model, 0));
                        auto entityModelIndex = std::get<2>(*entityInsert) = static_cast<uint32_t>(std::distance(std::begin(entityModelList), entityInsert));

						halfSizeXList[entityModelIndex] = halfSize.x;
						halfSizeYList[entityModelIndex] = halfSize.y;
						halfSizeZList[entityModelIndex] = halfSize.z;
						for (size_t element = 0; element < 16; ++element)
						{
                            transformList[element][entityModelIndex] = centerTransform.data[element];
						}
					});
				}
			});

			visibilityList.resize(bufferedModelCount);
            Math::SIMD::cullOrientedBoundingBoxes(viewMatrix, projectionMatrix, static_cast<uint32_t>(bufferedModelCount), halfSizeXList, halfSizeYList, halfSizeZList, transformList, visibilityList);

            const auto visibleModelCount = std::accumulate(std::begin(entityModelList), std::end(entityModelList), size_t(0), [&](size_t count, auto const &entitySearch) -> size_t
            {
                return (count + (visibilityList[std::get<2>(entitySearch)] ? 1 : 0));
            });

            if (!entityModelList.empty() && visibleModelCount == 0)
            {
                for (auto const &entitySearch : entityModelList)
                {
                    visibilityList[std::get<2>(entitySearch)] = true;
                }

                if ((modelQueueFrameCounter % 240) == 0)
                {
                    getContext()->log(Context::Warning, "ModelProcessor culling fallback: forcing model visibility (models={})", entityModelList.size());
                }
            }

            std::for_each(std::execution::par, std::begin(entityModelList), std::end(entityModelList), [&](auto &entitySearch) -> void
			{
				if (visibilityList[std::get<2>(entitySearch)])
				{
					Plugin::Entity * const entity = std::get<0>(entitySearch);
					auto model = std::get<1>(entitySearch);

					auto &transformComponent = entity->getComponent<Components::Transform>();
					auto modelViewMatrix(transformComponent.getScaledMatrix() * viewMatrix);

					std::for_each(std::execution::par, std::begin(model->meshList), std::end(model->meshList), [&](Group::Model::Mesh const &mesh) -> void
					{
						auto &meshMap = renderList[mesh.material];
						auto &instanceList = meshMap[&mesh];
						instanceList.push_back(modelViewMatrix);
					});
				}
			});

			std::atomic_size_t maximumInstanceCount = 0;
            std::atomic_size_t queuedBatchCount = 0;
            std::for_each(std::begin(renderList), std::end(renderList), [&](auto &materialPair) -> void
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

                std::vector<DrawData> drawDataList;
                drawDataList.reserve(materialMap.size());
                std::vector<Math::Float4x4> instanceList;
                instanceList.reserve(materialInstanceCount);
				for (auto &levelPair : materialMap)
				{
					auto level = levelPair.first;
					if (level)
					{
						auto &levelInstanceList = levelPair.second;
                        drawDataList.push_back(DrawData(static_cast<uint32_t>(instanceList.size()), static_cast<uint32_t>(levelInstanceList.size()), level));
						instanceList.insert(std::end(instanceList), std::begin(levelInstanceList), std::end(levelInstanceList));
						levelInstanceList.clear();
					}
				}

                updateMaximumValue(maximumInstanceCount, instanceList.size());
                queuedBatchCount.fetch_add(1);
				renderer->queueDrawCall(visual, material, std::move([this, drawDataList = move(drawDataList), instanceList = move(instanceList)](Render::Device::Context *videoContext) -> void
				{
					Math::Float4x4 *instanceData = nullptr;
					if (videoDevice->mapBuffer(instanceBuffer.get(), instanceData))
					{
						std::copy(std::begin(instanceList), std::end(instanceList), instanceData);
						videoDevice->unmapBuffer(instanceBuffer.get());
						videoContext->setVertexBufferList({ instanceBuffer.get() }, 4);
						for (auto const &drawData : drawDataList)
						{
							if (drawData.data)
							{
								auto &level = *drawData.data;
								resources->setVertexBufferList(videoContext, level.vertexBufferList, 0);
                                if (level.indexBuffer)
                                {
                                    resources->setIndexBuffer(videoContext, level.indexBuffer, 0);
                                    resources->drawInstancedIndexedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, level.indexCount, 0, 0);
                                }
                                else
                                {
                                    resources->drawInstancedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, level.vertexCount, 0);
                                }
							}
						}
					}
				}));
			});

			if (instanceBuffer->getDescription().count < maximumInstanceCount)
			{
				instanceBuffer = nullptr;
				Render::Buffer::Description instanceDescription;
                instanceDescription.name = "model:instances";
				instanceDescription.stride = sizeof(Math::Float4x4);
                instanceDescription.count = static_cast<uint32_t>(maximumInstanceCount.load());
				instanceDescription.type = Render::Buffer::Type::Vertex;
				instanceDescription.flags = Render::Buffer::Flags::Mappable;
				instanceBuffer = videoDevice->createBuffer(instanceDescription);
			}

            if ((modelQueueFrameCounter % 120) == 0)
            {
                getContext()->log(Context::Info,
                    "ModelProcessor frame {}: entities={}, visibleEntities={}, models={}, visibleModels={}, queuedBatches={}, maxInstances={}",
                    modelQueueFrameCounter,
                    entityDataList.size(),
                    visibleEntityCount,
                    entityModelList.size(),
                    visibleModelCount,
                    queuedBatchCount.load(),
                    maximumInstanceCount.load());
            }
		}
	};

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

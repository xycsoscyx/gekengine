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
#include "GEK/API/Core.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Resources.hpp"
#include "GEK/API/Editor.hpp"
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
        FileSystem::Path modelsPath;

    public:
        Model(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
            , modelsPath(context->findDataPath("models"s))
        {
        }

        // Plugin::Component
        void save(Components::Model const * const data, JSON &exportData) const
        {
            exportData = data->name;
        }

        void load(Components::Model * const data, JSON const &importData)
        {
            data->name = evaluate(importData, String::Empty);
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
                    std::function<bool(FileSystem::Path const &)> searchDirectory;
                    searchDirectory = ([&](FileSystem::Path const &filePath) -> bool
                    {
                        if (filePath.isDirectory())
                        {
                            filePath.findFiles(searchDirectory);
                        }
                        else if (filePath.getExtension() == ".gek")
                        {
                            auto modelPath = filePath.getParentPath().getString();
                            String::Replace(modelPath, modelsPath.getString(), "");
                            modelList.push_back(modelPath.substr(1));
                            return false;
                        }

                        return true;
                    });

                    modelsPath.findFiles(searchDirectory);
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
                        *outputText = modelList[index - 1].data();
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

        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
			Math::Float3 tangent;
			Math::Float3 biTangent;
			Math::Float3 normal;
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
                    std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(5);
                    ResourceHandle indexBuffer;
                    uint32_t indexCount = 0;
                };

                Shapes::AlignedBox boundingBox;
                std::vector<Mesh> meshList;
            };

            std::vector<Model> modelList;
            Shapes::AlignedBox boundingBox;
        };

        struct Data
        {
            Group *group = nullptr;
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
        Video::Device *videoDevice = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;
        Edit::Events *events = nullptr;

        VisualHandle visual;
        Video::BufferPtr instanceBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Group> groupMap;

        std::vector<float, AlignedAllocator<float, 16>> halfSizeXList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeYList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeZList;
        std::vector<float, AlignedAllocator<float, 16>> transformList[16];
        std::vector<bool> visibilityList;

        using EntityDataList = concurrency::concurrent_vector<std::tuple<Plugin::Entity * const, Data const *, uint32_t>>;
        using EntityModelList = concurrency::concurrent_vector<std::tuple<Plugin::Entity * const, Group::Model const *, uint32_t>>;
        EntityDataList entityDataList;
        EntityModelList entityModelList;

        using InstanceList = concurrency::concurrent_vector<Math::Float4x4>;
        using MeshInstanceMap = concurrency::concurrent_unordered_map<const Group::Model::Mesh *, InstanceList>;
        using MaterialMeshMap = concurrency::concurrent_unordered_map<MaterialHandle, MeshInstanceMap>;
        MaterialMeshMap renderList;

    public:
        ModelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , videoDevice(core->getRenderer()->getVideoDevice())
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
            , loadPool(5)
        {
            assert(core);
            assert(videoDevice);
			assert(population);
            assert(resources);
            assert(renderer);

            LockedWrite{ std::cout } << "Initializing model system";

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

        Task scheduleLoadData(std::string name, FileSystem::Path filePath, ModelProcessor::Group &group, ModelProcessor::Group::Model &model)
        {
            co_await loadPool.schedule();

            auto fileName(filePath.getFileName());

            static const std::vector<uint8_t> EmptyBuffer;
            std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer));

            Header* header = (Header*)buffer.data();
            if (buffer.size() < (sizeof(Header) + (sizeof(Header::Mesh) * header->meshCount)))
            {
                LockedWrite{ std::cerr } << "Model file too small to contain mesh headers: " << filePath.getString();
                co_return;
            }

            LockedWrite{ std::cout } << "Group " << name << ", loading model " << fileName << ": " << header->meshCount << " meshes";

            model.boundingBox = header->boundingBox;
            group.boundingBox.extend(model.boundingBox.minimum);
            group.boundingBox.extend(model.boundingBox.maximum);
            model.meshList.resize(header->meshCount);
            Unpacker unpacker((uint8_t*)&header->meshList[header->meshCount]);
            for (uint32_t meshIndex = 0; meshIndex < header->meshCount; ++meshIndex)
            {
                Header::Mesh& meshHeader = header->meshList[meshIndex];
                Group::Model::Mesh& mesh = model.meshList[meshIndex];

                mesh.material = resources->loadMaterial(meshHeader.material);

                Video::Buffer::Description indexBufferDescription;
                indexBufferDescription.format = Video::Format::R16_UINT;
                indexBufferDescription.count = (meshHeader.faceCount * 3);
                indexBufferDescription.type = Video::Buffer::Type::Index;
                mesh.indexBuffer = resources->createBuffer(std::format("model:{}.{}.{}:indices", meshIndex, fileName, name), indexBufferDescription, unpacker.readBlock<Face>(meshHeader.faceCount));

                Video::Buffer::Description vertexBufferDescription;
                vertexBufferDescription.stride = sizeof(Math::Float3);
                vertexBufferDescription.count = meshHeader.vertexCount;
                vertexBufferDescription.type = Video::Buffer::Type::Vertex;
                mesh.vertexBufferList[0] = resources->createBuffer(std::format("model:{}.{}.{}:positions", meshIndex, fileName, name), vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));

                vertexBufferDescription.stride = sizeof(Math::Float2);
                mesh.vertexBufferList[1] = resources->createBuffer(std::format("model:{}.{}.{}:texcoords", meshIndex, fileName, name), vertexBufferDescription, unpacker.readBlock<Math::Float2>(meshHeader.vertexCount));

                vertexBufferDescription.stride = sizeof(Math::Float3);
                mesh.vertexBufferList[2] = resources->createBuffer(std::format("model:{}.{}.{}:tangents", meshIndex, fileName, name), vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));

                vertexBufferDescription.stride = sizeof(Math::Float3);
                mesh.vertexBufferList[3] = resources->createBuffer(std::format("model:{}.{}.{}:bitangents", meshIndex, fileName, name), vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));

                vertexBufferDescription.stride = sizeof(Math::Float3);
                mesh.vertexBufferList[4] = resources->createBuffer(std::format("model:{}.{}.{}:normals", meshIndex, fileName, name), vertexBufferDescription, unpacker.readBlock<Math::Float3>(meshHeader.vertexCount));

                mesh.indexCount = indexBufferDescription.count;
            }

            LockedWrite{ std::cout } << "Group " << name << ", mesh " << fileName << " successfully loaded";
        }

        Task scheduleLoadGroup(std::string name, ModelProcessor::Group &group)
        {
            LockedWrite{ std::cout } << "Queueing group for load: " << name;

            co_await loadPool.schedule();

            std::vector<FileSystem::Path> modelPathList;
            auto groupPath(getContext()->findDataPath(FileSystem::CreatePath("models", name)));
            groupPath.findFiles([&](FileSystem::Path const& filePath) -> bool
            {
                std::string fileName(filePath.getString());
                if (filePath.isFile() && String::GetLower(filePath.getExtension()) == ".gek")
                {
                    static const std::vector<uint8_t> EmptyBuffer;
                    std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer, sizeof(Header)));
                    if (buffer.size() < sizeof(Header))
                    {
                        LockedWrite{ std::cerr } << "Model file too small to contain header: " << fileName;
                        return true;
                    }

                    Header* header = (Header*)buffer.data();
                    if (header->identifier != *(uint32_t*)"GEKX")
                    {
                        LockedWrite{ std::cerr } << "Unknown model file identifier encountered (requires: GEKX, has: " << header->identifier << "): " << fileName;
                        return true;
                    }

                    if (header->type != 0)
                    {
                        LockedWrite{ std::cerr } << "Unsupported model type encountered (requires: 0, has: " << header->type << "): " << fileName;
                        return true;
                    }

                    if (header->version != 8)
                    {
                        LockedWrite{ std::cerr } << "Unsupported model version encountered (requires: 8, has: " << header->version << "): " << fileName;
                        return true;
                    }

                    modelPathList.push_back(filePath);
                }

                return true;
            });

            if (modelPathList.empty())
            {
                LockedWrite{ std::cerr } << "No models found for group: " << name;
            }

            group.modelList.resize(modelPathList.size());
            for (size_t modelIndex = 0; modelIndex < modelPathList.size(); ++modelIndex)
            {
                auto& model = group.modelList[modelIndex];
                auto& filePath = modelPathList[modelIndex];
                scheduleLoadData(name, filePath, group, model);
            }

            LockedWrite{ std::cout } << "Group " << name << " successfully queued";
        }

        void addEntity(Plugin::Entity * const entity)
        {
            EntityProcessor::addEntity(entity, [&](bool isNewInsert, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                static const Group BlankGroup;
                auto pair = groupMap.insert(std::make_pair(GetHash(modelComponent.name), BlankGroup));
                if (pair.second)
                {
                    scheduleLoadGroup(modelComponent.name, pair.first->second);
                }

                data.group = &pair.first->second;
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
            if (modelSearch != std::end(groupMap))
            {
                return modelSearch->second.boundingBox;
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

        // Plugin::Renderer Slots
        void onQueueDrawCalls(Shapes::Frustum const &viewFrustum, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix)
        {
            assert(renderer);

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
			entityDataList.reserve(entityCount);
			parallelListEntities([&](Plugin::Entity * const entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
			{
				auto group = data.group;
				auto matrix(transformComponent.getMatrix());
				matrix.translation.xyz += group->boundingBox.getCenter();
				auto halfSize(group->boundingBox.getHalfSize() * transformComponent.scale);

				auto entityInsert = entityDataList.push_back(std::make_tuple(entity, &data, 0));
				auto entityIndex = std::get<2>(*entityInsert) = std::distance(std::begin(entityDataList), entityInsert);

				halfSizeXList[entityIndex] = halfSize.x;
				halfSizeYList[entityIndex] = halfSize.y;
				halfSizeZList[entityIndex] = halfSize.z;
				for (size_t element = 0; element < 16; ++element)
				{
					transformList[element][entityIndex] = matrix.data[element];
				}
			});

			visibilityList.resize(bufferedEntityCount);
			Math::SIMD::cullOrientedBoundingBoxes(viewMatrix, projectionMatrix, bufferedEntityCount, halfSizeXList, halfSizeYList, halfSizeZList, transformList, visibilityList);

			// Cull by model inside group
			const auto modelCount = std::accumulate(std::begin(entityDataList), std::end(entityDataList), 0U, [this](auto count, auto const &entitySearch) -> auto
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
			entityModelList.reserve(bufferedModelCount);
			concurrency::parallel_for_each(std::begin(entityDataList), std::end(entityDataList), [&](auto &entitySearch) -> void
			{
				auto entityDataIndex = std::get<2>(entitySearch);
				if (visibilityList[entityDataIndex])
				{
					auto entity = std::get<0>(entitySearch);
					auto data = std::get<1>(entitySearch);
					auto group = data->group;

					auto &transformComponent = entity->getComponent<Components::Transform>();
					auto matrix(transformComponent.getMatrix());

					concurrency::parallel_for_each(std::begin(group->modelList), std::end(group->modelList), [&](Group::Model const &model) -> void
					{
						auto halfSize(group->boundingBox.getHalfSize() * transformComponent.scale);
						auto center = Math::Float4x4::MakeTranslation(model.boundingBox.getCenter());

						auto entityInsert = entityModelList.push_back(std::make_tuple(entity, &model, 0));
						auto entityModelIndex = std::get<2>(*entityInsert) = std::distance(std::begin(entityModelList), entityInsert);

						halfSizeXList[entityModelIndex] = halfSize.x;
						halfSizeYList[entityModelIndex] = halfSize.y;
						halfSizeZList[entityModelIndex] = halfSize.z;
						for (size_t element = 0; element < 16; ++element)
						{
							transformList[element][entityModelIndex] = (matrix.data[element] + center.data[element]);
						}
					});
				}
			});

			visibilityList.resize(bufferedModelCount);
			Math::SIMD::cullOrientedBoundingBoxes(viewMatrix, projectionMatrix, bufferedModelCount, halfSizeXList, halfSizeYList, halfSizeZList, transformList, visibilityList);

			concurrency::parallel_for_each(std::begin(entityModelList), std::end(entityModelList), [&](auto &entitySearch) -> void
			{
				if (visibilityList[std::get<2>(entitySearch)])
				{
					auto entity = std::get<0>(entitySearch);
					auto model = std::get<1>(entitySearch);

					auto &transformComponent = entity->getComponent<Components::Transform>();
					auto modelViewMatrix(transformComponent.getScaledMatrix() * viewMatrix);

					concurrency::parallel_for_each(std::begin(model->meshList), std::end(model->meshList), [&](Group::Model::Mesh const &mesh) -> void
					{
						auto &meshMap = renderList[mesh.material];
						auto &instanceList = meshMap[&mesh];
						instanceList.push_back(modelViewMatrix);
					});
				}
			});

			size_t maximumInstanceCount = 0;
			concurrency::parallel_for_each(std::begin(renderList), std::end(renderList), [&](auto &materialPair) -> void
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
				for (auto &levelPair : materialMap)
				{
					auto level = levelPair.first;
					if (level)
					{
						auto &levelInstanceList = levelPair.second;
						drawDataList.push_back(DrawData(instanceList.size(), levelInstanceList.size(), level));
						instanceList.insert(std::end(instanceList), std::begin(levelInstanceList), std::end(levelInstanceList));
						levelInstanceList.clear();
					}
				}

				InterlockedExchange(&maximumInstanceCount, std::max(maximumInstanceCount, instanceList.size()));
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
			});

			if (instanceBuffer->getDescription().count < maximumInstanceCount)
			{
				instanceBuffer = nullptr;
				Video::Buffer::Description instanceDescription;
				instanceDescription.stride = sizeof(Math::Float4x4);
				instanceDescription.count = maximumInstanceCount;
				instanceDescription.type = Video::Buffer::Type::Vertex;
				instanceDescription.flags = Video::Buffer::Flags::Mappable;
				instanceBuffer = videoDevice->createBuffer(instanceDescription);
				instanceBuffer->setName("model:instances"sv);
			}
		}
	};

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek

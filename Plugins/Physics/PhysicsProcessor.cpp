#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

//ndVector ndVector::m_zero(ndFloat32(0.0f));
//ndVector ndVector::m_one(ndFloat32(1.0f));
//ndVector ndVector::m_wOne(ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(1.0f));

namespace Gek
{
    namespace Physics
    {
        extern BodyPtr createPlayerBody(Plugin::Core* core, Plugin::Population* population, World* World, Plugin::Entity* const entity);
        extern BodyPtr createRigidBody(World* world, Plugin::Entity* const entity);

        class BufferReader
        {
        private:
            uint8_t* buffer = nullptr;
            size_t index = 0;

        public:
            BufferReader(uint8_t* buffer)
                : buffer(buffer)
            {
            }

            template <typename TYPE>
            TYPE* read(uint32_t count = 1)
            {
                TYPE* data = (TYPE*)(buffer + index);
                index += (sizeof(TYPE) * count);
                return data;
            }
        };

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
            , public Plugin::Processor
            , public World
        {
        public:
            struct Header
            {
                uint32_t identifier = 0;
                uint16_t type = 0;
                uint16_t version =  0;
            };

			struct HullHeader : public Header
			{
                uint32_t pointCount;
			};

            struct TreeHeader : public Header
            {
                struct Material
                {
                    char name[64];
                };

                struct Mesh
                {
                    uint32_t materialIndex;
                    uint32_t faceCount;
                    uint32_t pointCount;
                };

                struct Face
                {
                    int32_t indices[3];
                };

                uint32_t materialCount;
                uint32_t meshCount;
            };

            struct Vertex
            {
                Math::Float3 position;
                Math::Float2 texCoord;
                Math::Float3 normal;
            };

            struct Material
            {
                uint32_t firstVertex;
                uint32_t firstIndex;
                uint32_t indexCount;
            };

            class NewtonWorld
                : public ndWorld
            {
            public:
                NewtonWorld(Processor* processor)
                {
                }
            };

        private:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
            Plugin::Renderer *renderer = nullptr;
            Edit::Events *events = nullptr;

            concurrency::concurrent_vector<Surface> surfaceList;
            concurrency::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            NewtonWorld* newtonWorld = nullptr;

            concurrency::concurrent_unordered_map<Plugin::Entity*, Physics::Body *> entityBodyMap;
            concurrency::concurrent_unordered_map<Hash, ndShape *> shapeMap;

        public:
            Processor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(core->getPopulation())
                , renderer(core->getRenderer())
            {
                assert(core);
                assert(population);
                assert(renderer);

                core->onInitialized.connect(this, &Processor::onInitialized);
                core->onShutdown.connect(this, &Processor::onShutdown);
                population->onReset.connect(this, &Processor::onReset);
                population->onEntityCreated.connect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.connect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.connect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.connect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].connect(this, &Processor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Processor::onShowUserInterface);

                onReset();
            }

            void clear(void)
            {
                if (newtonWorld)
                {
                    newtonWorld->Sync();

                    surfaceList.clear();
                    surfaceIndexMap.clear();

                    newtonWorld->CleanUp();

                    entityBodyMap.clear();
                    shapeMap.clear();

                    delete newtonWorld;
                    newtonWorld = nullptr;
                }
            }

            ndShape* loadShape(Components::Model const& modelComponent)
            {
                ndShape* shape = nullptr;
                auto hash = GetHash(modelComponent.name);
                auto shapeSearch = shapeMap.find(hash);
                if (shapeSearch == std::end(shapeMap))
                {
                    getContext()->log(Context::Info, "Loading physics model: {}", modelComponent.name);

                    if (modelComponent.name == "#cube")
                    {
                        shape = new ndShapeBox(1.0f, 1.0f, 1.0f);
                    }
                    else if (modelComponent.name == "#sphere")
                    {
                        shape = new ndShapeSphere(1.0f);
                    }
                    else
                    {
                        auto filePath = getContext()->findDataPath(FileSystem::CreatePath("physics", modelComponent.name).withExtension(".gek"));
                        std::vector<uint8_t> buffer(FileSystem::Load(filePath));
                        if (buffer.size() < sizeof(Header))
                        {
                            getContext()->log(Context::Error, "File too small to be physics model: {}", modelComponent.name);
                            return nullptr;
                        }

                        BufferReader reader(buffer.data());
                        Header* header = reader.read<Header>(0);
                        if (header->identifier != *(uint32_t*)"GEKX")
                        {
                            getContext()->log(Context::Error, "Unknown model file identifier encountered: {}", modelComponent.name);
                            return nullptr;
                        }

                        if (header->version != 3)
                        {
                            getContext()->log(Context::Error, "Unsupported model version encountered (requires: 2, has: {}): {}", header->version, modelComponent.name);
                            return nullptr;
                        }

                        if (header->type == 1)
                        {
                            getContext()->log(Context::Info, "Loading hull: {}", modelComponent.name);

                            HullHeader* hullHeader = reader.read<HullHeader>();
                            Math::Float3* points = reader.read<Math::Float3>(hullHeader->pointCount);
                            shape = new ndShapeConvexHull(hullHeader->pointCount, sizeof(Math::Float3), 0.0f, points->data);
                        }
                        else if (header->type == 2)
                        {
                            getContext()->log(Context::Info, "Loading tree: {}", modelComponent.name);
                            /*
                            TreeHeader* treeHeader = reader.read<TreeHeader>();

                            for (uint32_t materialIndex = 0; materialIndex < treeHeader->materialCount; ++materialIndex)
                            {
                                TreeHeader::Material* materialHeader = reader.read<TreeHeader::Material>();
                                loadSurface(materialHeader->name);
                            }

                            for (uint32_t meshIndex = 0; meshIndex < treeHeader->meshCount; meshIndex++)
                            {
                                TreeHeader::Mesh* mesh = reader.read<TreeHeader::Mesh>();
                                TreeHeader::Face* faces = reader.read<TreeHeader::Face>(mesh->faceCount);
                                Math::Float3* points = reader.read<Math::Float3>(mesh->pointCount);

                                for (uint32_t faceIndex = 0; faceIndex < mesh->faceCount; faceIndex++)
                                {
                                }
                            }*/
                        }
                        else
                        {
                            getContext()->log(Context::Error, "Unsupported model type encountered: {}", modelComponent.name);
                            return nullptr;
                        }
                    }

                    getContext()->log(Context::Info, "Physics shape successfully loaded: {}", modelComponent.name);
                }

                if (shape)
                {
                    shapeMap[hash] = shape;
                }

                return shape;
            }

            concurrency::critical_section criticalSection;
            void addEntity(Plugin::Entity * const entity)
            {
                BodyPtr body;
                if (entity->hasComponent<Components::Transform>())
                {
                    concurrency::critical_section::scoped_lock lock(criticalSection);
                    if (entity->hasComponents<Components::Model, Components::Scene>())
                    {
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                    }
                    else if (entity->hasComponents<Components::Physical>())
                    {
                        auto &physicalComponent = entity->getComponent<Components::Physical>();
                        if (entity->hasComponent<Components::Player>())
                        {
                            body = createPlayerBody(core, population, this, entity);
                        }
                        else if (entity->hasComponent<Components::Model>())
                        {
                            auto const& modelComponent = entity->getComponent<Components::Model>();
                            auto shape = loadShape(modelComponent);
                            if (shape)
                            {
                                body = createRigidBody(this, entity);
                                if (body)
                                {
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                                }
                            }
                        }
                    }
                }

                if (body)
                {
                    if (newtonWorld)
                    {
                        ndSharedPtr<ndBody> sharedBody(body->getAsNewtonBody());

                        auto& transformComponent = entity->getComponent<Components::Transform>();
                        sharedBody->SetMatrix(transformComponent.getScaledMatrix().data);

                        newtonWorld->AddBody(sharedBody);
                    }

                    entityBodyMap[entity] = body.release();
                }
            }

            void removeEntity(Plugin::Entity * const entity)
            {
                auto entitySearch = entityBodyMap.find(entity);
                if (entitySearch != std::end(entityBodyMap))
                {
                    newtonWorld->RemoveBody(entitySearch->second->getAsNewtonBody());
                    entityBodyMap.unsafe_erase(entitySearch);

                }
            }

            // Plugin::Core
            void onInitialized(void)
            {
                core->listProcessors([&](Plugin::Processor *processor) -> void
                {
                    auto castCheck = dynamic_cast<Edit::Events *>(processor);
                    if (castCheck)
                    {
                        (events = castCheck)->onModified.connect(this, &Processor::onModified);
                    }                    
                });
            }

            void onShutdown(void)
            {
                if (events)
                {
                    events->onModified.disconnect(this, &Processor::onModified);
                }

                renderer->onShowUserInterface.disconnect(this, &Processor::onShowUserInterface);
                population->onReset.disconnect(this, &Processor::onReset);
                population->onEntityCreated.disconnect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.disconnect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.disconnect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.disconnect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].disconnect(this, &Processor::onUpdate);

                clear();
            }

            // Plugin::Editor Slots
            void onModified(Plugin::Entity* const entity, Hash type)
            {
                auto bodySearch = entityBodyMap.find(entity);
                if (bodySearch == std::end(entityBodyMap))
                {
                    return;
                }

                auto body = bodySearch->second;
                if (type == Components::Transform::GetIdentifier())
                {
                    auto entitySearch = entityBodyMap.find(entity);
                    if (entitySearch != std::end(entityBodyMap))
                    {
                        auto const& transformComponent = entity->getComponent<Components::Transform>();
                        auto matrix(transformComponent.getScaledMatrix());
                        body->getAsNewtonBody()->SetMatrix(matrix.data);
                    }
                }
                else if (type == Components::Model::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Physical>())
                    {
                        auto const& physicalComponent = entity->getComponent<Components::Physical>();
                        auto const& modelComponent = entity->getComponent<Components::Model>();
                        auto shape = loadShape(modelComponent);
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                    }
                }
                else if (type == Components::Physical::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Model>())
                    {
                        auto const& physicalComponent = entity->getComponent<Components::Physical>();
                        auto& shapeInstance = body->getAsNewtonBody()->GetAsBodyDynamic()->GetCollisionShape();
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, shapeInstance);
                    }
                }
            }

            // Plugin::Core Slots
            void onShowUserInterface(void)
            {
            }

            // Plugin::Population Slots
            void onReset(void)
            {
                clear();
                newtonWorld = new NewtonWorld(this);

                newtonWorld->Sync();
                newtonWorld->SetSubSteps(2);
                newtonWorld->SetSolverIterations(1);
                newtonWorld->SetThreadCount(4);
                //newtonWorld->SelectSolver(m_solverMode);
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
                if (!entity->hasComponents<Components::Transform, Components::Physical>())
                {
                    removeEntity(entity);
                }
            }

            void onUpdate(float frameTime)
            {
				bool editorActive = core->getOption("editor", "active", false);
				if (frameTime > 0.0f && !editorActive)
				{
					static constexpr float StepTime = (1.0f / 60.0f);
					while (frameTime > 0.0f)
					{
                        if (newtonWorld)
                        {
                            newtonWorld->Update(StepTime);
                            newtonWorld->Sync();
                        }

                        frameTime -= StepTime;
					};
				}
            }

            // Newton::World
            Math::Float3 getGravity(Math::Float3 const *position)
            {
                const Math::Float3 DefaultGravity(0.0f, -32.174f, 0.0f);

                auto localGravity = DefaultGravity;
                if (position)
                {
                }

                return localGravity;
            }

            uint32_t loadSurface(std::string const &surfaceName)
            {
                uint32_t surfaceIndex = 0;

                auto hash = GetHash(surfaceName);
                auto surfaceSearch = surfaceIndexMap.find(hash);
                if (surfaceSearch != std::end(surfaceIndexMap))
                {
                    surfaceIndex = surfaceSearch->second;
                }
                else
                {
                    surfaceIndexMap[hash] = 0;

                    JSON::Object materialNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("materials", surfaceName).withExtension(".json")));
                    auto surfaceNode = materialNode["surface"];
                    if (surfaceNode.is_object())
                    {
                        Surface surface;
                        surface.ghost = JSON::Value(surfaceNode, "ghost", surface.ghost);
                        surface.staticFriction = JSON::Value(surfaceNode, "static_friction", surface.staticFriction);
                        surface.kineticFriction = JSON::Value(surfaceNode, "kinetic_friction", surface.kineticFriction);
                        surface.elasticity = JSON::Value(surfaceNode, "elasticity", surface.elasticity);
                        surface.softness = JSON::Value(surfaceNode, "softness", surface.softness);

                        surfaceIndex = surfaceList.size();
                        surfaceList.push_back(surface);
                        surfaceIndexMap[hash] = surfaceIndex;
                    }
                }

                return surfaceIndex;
            }

            const Surface & getSurface(uint32_t surfaceIndex) const
            {
                static const Surface DefaultSurface;
                return (surfaceIndex >= surfaceList.size() ? DefaultSurface : surfaceList[surfaceIndex]);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Processor)
    }; // namespace Physics
}; // namespace Gek

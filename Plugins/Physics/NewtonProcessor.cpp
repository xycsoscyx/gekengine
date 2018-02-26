#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Profiler.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Newton/Base.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

#include <Newton.h>

namespace Gek
{
    namespace Newton
    {
        extern EntityPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, NewtonWorld *newtonWorld, Plugin::Entity * const entity);
        extern EntityPtr createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Plugin::Entity * const entity);

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
            , public Plugin::Processor
            , public Newton::World
            , public Newton::Entity
        {
        public:
            struct Header
            {
                uint32_t identifier = 0;
                uint16_t type = 0;
                uint16_t version =  0;
                uint32_t newtonVersion = 0;
            };

			struct HullHeader : public Header
			{
				uint8_t serializationData[1];
			};

            struct TreeHeader : public Header
            {
                struct Material
                {
                    char name[64];
                };

                uint32_t materialCount;
                Material materialList[1];
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

        private:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
            Plugin::Renderer *renderer = nullptr;
            Edit::Events *events = nullptr;

            NewtonWorld *newtonWorld = nullptr;
            void *newtonListener = nullptr;

            NewtonCollision *newtonSceneCollision = nullptr;
            NewtonBody *newtonSceneBody = nullptr;

            concurrency::concurrent_vector<Surface> surfaceList;
            concurrency::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            concurrency::concurrent_unordered_map<std::size_t, NewtonCollision *> collisionMap;
            concurrency::concurrent_unordered_map<Plugin::Entity *, Newton::EntityPtr> entityMap;

            using SurfaceMap = std::unordered_map<uint32_t, uint32_t>;
            concurrency::concurrent_unordered_map<NewtonCollision *, SurfaceMap> sceneSurfaceMap;
            concurrency::concurrent_unordered_map<Plugin::Entity *, void *> sceneMap;


        public:
            Processor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(core->getPopulation())
                , renderer(core->getRenderer())
                , newtonWorld(NewtonCreate())
            {
                assert(core);
                assert(newtonWorld);

                NewtonSetSolverModel(newtonWorld, 4);
                NewtonWorldSetUserData(newtonWorld, static_cast<Newton::World *>(this));

                newtonListener = NewtonWorldAddListener(newtonWorld, "__gek_pre_listener__", this);
                assert(newtonListener);

                NewtonWorldListenerSetPreUpdateCallback(newtonWorld, newtonListener, newtonWorldPreUpdate);
                NewtonWorldListenerSetPostUpdateCallback(newtonWorld, newtonListener, newtonWorldPostUpdate);

                int defaultMaterialID = NewtonMaterialGetDefaultGroupID(newtonWorld);
                NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, newtonOnAABBOverlap, newtonOnContactFriction);

                core->onInitialized.connect(this, &Processor::onInitialized);
                core->onShutdown.connect(this, &Processor::onShutdown);
                population->onReset.connect(this, &Processor::onReset);
                population->onEntityCreated.connect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.connect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.connect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.connect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].connect(this, &Processor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Processor::onShowUserInterface);
            }

            NewtonCollision *loadCollision(const Components::Model &modelComponent)
            {
                NewtonCollision *newtonCollision = nullptr;

                auto hash = GetHash(modelComponent.name);
                auto collisionSearch = collisionMap.find(hash);
                if (collisionSearch != std::end(collisionMap))
                {
                    if (collisionSearch->second)
                    {
                        newtonCollision = collisionSearch->second;
                    }
                }
                else
                {
                    collisionMap[hash] = nullptr;

                    LockedWrite{ std::cout } << "Loading collision model: " << modelComponent.name;

					static const std::vector<uint8_t> EmptyBuffer;
                    auto filePath = getContext()->findDataPath(FileSystem::CombinePaths("physics", modelComponent.name).withExtension(".gek"));
                    std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer));
					if (buffer.size() < sizeof(Header))
					{
						LockedWrite{ std::cerr } << "File too small to be collision model: " << modelComponent.name;
						return nullptr;
					}

                    Header *header = (Header *)buffer.data();
                    if (header->identifier != *(uint32_t *)"GEKX")
                    {
						LockedWrite{ std::cerr } << "Unknown model file identifier encountered: " << modelComponent.name;
						return nullptr;
                    }

                    if (header->version != 2)
                    {
                        LockedWrite{ std::cerr } << "Unsupported model version encountered (requires: 2, has: " << header->version << "): " << modelComponent.name;
						return nullptr;
					}

                    if (header->newtonVersion != NewtonWorldGetVersion())
                    {
						LockedWrite{ std::cerr } << "Model created with different version of Newton Dynamics (requires: " << NewtonWorldGetVersion() << ", has: " << header->newtonVersion << "): " << modelComponent.name;
						return nullptr;
					}

                    struct DeSerializationData
                    {
                        std::vector<uint8_t> &buffer;
                        std::size_t offset;

                        DeSerializationData(std::vector<uint8_t> &buffer, uint8_t *start)
                            : buffer(buffer)
                            , offset(start - &buffer.at(0))
                        {
                        }
                    };

                    auto deSerializeCollision = [](void* const serializeHandle, void* const buffer, int size) -> void
                    {
                        auto data = (DeSerializationData *)serializeHandle;
                        memcpy(buffer, &data->buffer.at(data->offset), size);
                        data->offset += size;
                    };

                    if (header->type == 1)
                    {
						LockedWrite{ std::cout } << "Loading hull collision: " << modelComponent.name;

						HullHeader *hullHeader = (HullHeader *)header;
                        DeSerializationData data(buffer, (uint8_t *)&hullHeader->serializationData[0]);
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, &data);
                    }
                    else if (header->type == 2)
                    {
						LockedWrite{ std::cout } << "Loading tree collision: " << modelComponent.name;
						
						TreeHeader *treeHeader = (TreeHeader *)header;
                        DeSerializationData data(buffer, (uint8_t *)&treeHeader->materialList[treeHeader->materialCount]);
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, &data);

                        auto &surfaceMap = sceneSurfaceMap[newtonCollision];
                        for (uint32_t materialIndex = 0; materialIndex < treeHeader->materialCount; ++materialIndex)
                        {
                            TreeHeader::Material &materialHeader = treeHeader->materialList[materialIndex];
                            surfaceMap[materialIndex] = loadSurface(materialHeader.name);
                        }
                    }
                    else
                    {
						LockedWrite{ std::cerr } << "Unsupported model type encountered: " << modelComponent.name;
						return nullptr;
                    }

                    if (newtonCollision == nullptr)
                    {
						LockedWrite{ std::cerr } << "Unable to create model collision object: " << modelComponent.name;
						return nullptr;
					}

                    NewtonCollisionSetMode(newtonCollision, true);
                    NewtonCollisionSetScale(newtonCollision, 1.0f, 1.0f, 1.0f);
                    NewtonCollisionSetMatrix(newtonCollision, Math::Float4x4::Identity.data);
                    collisionMap[hash] = newtonCollision;

					LockedWrite{ std::cout } << "Collision model successfully loaded: " << modelComponent.name;
				}

                return newtonCollision;
            }

            void createSceneCollision(void)
            {
                if (!newtonSceneCollision)
                {
                    newtonSceneCollision = NewtonCreateSceneCollision(newtonWorld, 1);
                    assert(newtonSceneCollision && "Unable to create scene collision");

                    NewtonCollisionSetMode(newtonSceneCollision, true);
                    NewtonCollisionSetScale(newtonSceneCollision, 1.0f, 1.0f, 1.0f);
                    NewtonCollisionSetMatrix(newtonSceneCollision, Math::Float4x4::Identity.data);

                    newtonSceneBody = NewtonCreateDynamicBody(newtonWorld, newtonSceneCollision, Math::Float4x4::Identity.data);
                    assert(newtonSceneBody && "Unable to create scene static body");

                    NewtonBodySetUserData(newtonSceneBody, dynamic_cast<Newton::Entity *>(this));
                    NewtonBodySetMassProperties(newtonSceneBody, 0.0f, newtonSceneCollision);
                    NewtonBodySetCollidable(newtonSceneBody, true);
                    NewtonBodySetAutoSleep(newtonSceneBody, true);
                }
            }

            concurrency::critical_section criticalSection;
            void addEntity(Plugin::Entity * const entity)
            {
                if (entity->hasComponent<Components::Transform>())
                {
                    concurrency::critical_section::scoped_lock lock(criticalSection);
                    auto &transformComponent = entity->getComponent<Components::Transform>();
                    if (entity->hasComponents<Components::Model, Components::Scene>())
                    {
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                        auto newtonCollision = loadCollision(modelComponent);
                        if (newtonCollision)
                        {
                            createSceneCollision();
                            NewtonSceneCollisionBeginAddRemove(newtonSceneCollision);
                            auto collisionNode = NewtonSceneCollisionAddSubCollision(newtonSceneCollision, newtonCollision);
                            if (collisionNode)
                            {
                                NewtonSceneCollisionSetSubCollisionMatrix(newtonSceneCollision, collisionNode, transformComponent.getMatrix().data);
                                //NewtonCollisionSetScale(subCollision, transformComponent.scale.x, transformComponent.scale.y, transformComponent.scale.z);
                                auto subCollision = NewtonSceneCollisionGetCollisionFromNode(newtonSceneCollision, collisionNode);
                                if (subCollision)
                                {
                                    auto surfaceMapSearch = sceneSurfaceMap.find(newtonCollision);
                                    if (surfaceMapSearch != std::end(sceneSurfaceMap))
                                    {
                                        NewtonCollisionSetUserData(subCollision, &surfaceMapSearch->second);
                                    }
                                }

                                sceneMap.insert(std::make_pair(entity, collisionNode));
                            }

                            NewtonSceneCollisionEndAddRemove(newtonSceneCollision);
                            NewtonBodySetCollision(newtonSceneBody, newtonSceneCollision);
                        }
                    }
                    else if (entity->hasComponents<Components::Physical>())
                    {
                        auto &physicalComponent = entity->getComponent<Components::Physical>();
                        if (entity->hasComponent<Components::Player>())
                        {
                            auto playerBody(createPlayerBody(core, population, newtonWorld, entity));
                            if (playerBody)
                            {
                                NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                                entityMap[entity] = std::move(playerBody);
                            }
                        }
                        else if (entity->hasComponent<Components::Model>())
                        {
                            auto const &modelComponent = entity->getComponent<Components::Model>();
                            auto newtonCollision = loadCollision(modelComponent);
                            if (newtonCollision)
                            {
                                auto rigidBody(createRigidBody(newtonWorld, newtonCollision, entity));
                                if (rigidBody)
                                {
                                    NewtonBodySetTransformCallback(rigidBody->getNewtonBody(), newtonSetTransform);
                                    entityMap[entity] = std::move(rigidBody);
                                }
                            }
                        }
                    }
                }
            }

            void removeEntity(Plugin::Entity * const entity)
            {
                auto entitySearch = entityMap.find(entity);
                if (entitySearch != std::end(entityMap))
                {
                    NewtonDestroyBody(entitySearch->second->getNewtonBody());
                    auto newtonCollision = NewtonBodyGetCollision(entitySearch->second->getNewtonBody());
                    entityMap.unsafe_erase(entitySearch);
                }

                auto sceneSearch = sceneMap.find(entity);
                if (sceneSearch != std::end(sceneMap))
                {
                    NewtonSceneCollisionBeginAddRemove(newtonSceneCollision);
                    NewtonSceneCollisionRemoveSubCollision(newtonSceneCollision, sceneSearch->second);
                    NewtonSceneCollisionEndAddRemove(newtonSceneCollision);
                    NewtonBodySetCollision(newtonSceneBody, newtonSceneCollision);
                    sceneMap.unsafe_erase(sceneSearch);
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

                onReset();

                NewtonDestroy(newtonWorld);
                assert(NewtonGetMemoryUsed() == 0);
            }

            // Plugin::Editor Slots
            void onModified(Plugin::Entity * const entity, Hash type)
            {
                if (type == Components::Transform::GetIdentifier())
                {
                    auto &transformComponent = entity->getComponent<Components::Transform>();

                    auto entitySearch = entityMap.find(entity);
                    if (entitySearch != std::end(entityMap))
                    {
                        NewtonBodySetMatrix(entitySearch->second->getNewtonBody(), transformComponent.getMatrix().data);
                        NewtonBodySetCollisionScale(entitySearch->second->getNewtonBody(), transformComponent.scale.x, transformComponent.scale.y, transformComponent.scale.z);
                    }

                    auto sceneSearch = sceneMap.find(entity);
                    if (sceneSearch != std::end(sceneMap))
                    {
                        NewtonSceneCollisionSetSubCollisionMatrix(newtonSceneCollision, sceneSearch->second, transformComponent.getMatrix().data);
                    }
                }
                else if (type == Components::Model::GetIdentifier())
                {
                }
            }

            // Plugin::Core Slots
            void onShowUserInterface(void)
            {
            }

            // Plugin::Population Slots
            void onReset(void)
            {
                NewtonWaitForUpdateToFinish(newtonWorld);
                for (auto const &collisionPair : collisionMap)
                {
                    if (collisionPair.second)
                    {
                        NewtonDestroyCollision(collisionPair.second);
                    }
                }

                collisionMap.clear();
                newtonSceneBody = nullptr;
                if (newtonSceneCollision)
                {
                    NewtonDestroyCollision(newtonSceneCollision);
                    newtonSceneCollision = nullptr;
                }

                sceneMap.clear();
                sceneSurfaceMap.clear();
                entityMap.clear();
                surfaceList.clear();
                surfaceIndexMap.clear();
                NewtonDestroyAllBodies(newtonWorld);
                NewtonInvalidateCache(newtonWorld);
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
                assert(population);
                assert(newtonWorld);

                GEK_PROFILE_FUNCTION_SCOPE();

                bool editorActive = core->getOption("editor", "active").convert(false);
                if (frameTime > 0.0f && !editorActive)
                {
					static constexpr float StepTime = (1.0f / 120.0f);
                    while (frameTime > 0.0f)
                    {
                        NewtonUpdate(newtonWorld, std::min(frameTime, StepTime));
                        frameTime -= StepTime;
                    };

                    NewtonWaitForUpdateToFinish(newtonWorld);
                }
            }

            // Newton::Entity
            Plugin::Entity * const getEntity(void) const
            {
                return nullptr;
            }

            NewtonBody * const getNewtonBody(void) const
            {
                return newtonSceneBody;
            }

            uint32_t getSurface(Math::Float3 const &position, Math::Float3 const &normal)
            {
                return 0;
            }

            // Newton::World
            Math::Float3 getGravity(Math::Float3 const &position)
            {
                const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);
                return Gravity;
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
                    JSON::Instance materialNode = JSON::Load(getContext()->findDataPath(FileSystem::CombinePaths("materials", surfaceName).withExtension(".json")));
                    if (materialNode.getObject().has_member("surface"))
                    {
                        Surface surface;
                        auto surfaceNode = materialNode.get("surface");
                        surface.ghost = surfaceNode.get("ghost").convert(surface.ghost);
                        surface.staticFriction = surfaceNode.get("static_friction").convert(surface.staticFriction);
                        surface.kineticFriction = surfaceNode.get("kinetic_friction").convert(surface.kineticFriction);
                        surface.elasticity = surfaceNode.get("elasticity").convert(surface.elasticity);
                        surface.softness = surfaceNode.get("softness").convert(surface.softness);

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

            // Processor
            static void newtonWorldPreUpdate(const NewtonWorld* const world, void* const userData, float frameTime)
            {
                std::map<Newton::Entity *, float> updateMap;
                Processor *processor = static_cast<Processor *>(userData);
                for (auto const &entityPair : processor->entityMap)
                {
                    auto updatePair = (*updateMap.insert(std::make_pair(entityPair.second.get(), frameTime)).first);
                    NewtonDispachThreadJob(processor->newtonWorld, [](NewtonWorld* const world, void* const userData, int threadIndex) -> void
                    {
                        auto updatePair = static_cast<std::pair<Newton::Entity *, float> *>(userData);
                        updatePair->first->onPreUpdate(updatePair->second, threadIndex);
                    }, &updatePair);
                }

                NewtonSyncThreadJobs(processor->newtonWorld);
            }

            static void newtonWorldPostUpdate(const NewtonWorld* const world, void* const userData, float frameTime)
            {
                std::map<Newton::Entity *, float> updateMap;
                Processor *processor = static_cast<Processor *>(userData);
                for (auto const &entityPair : processor->entityMap)
                {
                    auto updatePair = (*updateMap.insert(std::make_pair(entityPair.second.get(), frameTime)).first);
                    NewtonDispachThreadJob(processor->newtonWorld, [](NewtonWorld* const world, void* const userData, int threadIndex) -> void
                    {
                        auto updatePair = static_cast<std::pair<Newton::Entity *, float> *>(userData);
                        updatePair->first->onPostUpdate(updatePair->second, threadIndex);
                    }, &updatePair);
                }

                NewtonSyncThreadJobs(processor->newtonWorld);
            }

            static void newtonSetTransform(const NewtonBody* const body, const float* const matrixData, int threadHandle)
            {
                Newton::Entity *newtonEntity = static_cast<Newton::Entity *>(NewtonBodyGetUserData(body));
                newtonEntity->onSetTransform(matrixData, threadHandle);
            }

            static int newtonOnAABBOverlap(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int threadHandle)
            {
                return 1;
            }

            static void newtonOnContactFriction(const NewtonJoint* contactJoint, float frameTime, int threadHandle)
            {
                const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
                const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);

                NewtonWorld *newtonWorld = NewtonBodyGetWorld(body0);
                Newton::World *EntityProcessor = static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld));
                Processor *processor = dynamic_cast<Processor *>(EntityProcessor);

                Newton::Entity *newtonEntity0 = static_cast<Newton::Entity *>(NewtonBodyGetUserData(body0));
                Newton::Entity *newtonEntity1 = static_cast<Newton::Entity *>(NewtonBodyGetUserData(body1));
                Plugin::Entity *entity0 = (newtonEntity0 ? newtonEntity0->getEntity() : nullptr);
                Plugin::Entity *entity1 = (newtonEntity1 ? newtonEntity1->getEntity() : nullptr);

                NewtonWorldCriticalSectionLock(newtonWorld, threadHandle);
                for (void* newtonContact = NewtonContactJointGetFirstContact(contactJoint); newtonContact; newtonContact = NewtonContactJointGetNextContact(contactJoint, newtonContact))
                {
                    NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);
                    //auto newtonCollision0 = NewtonContactGetCollision0(newtonContact);
                    //auto newtonCollision1 = NewtonContactGetCollision1(newtonContact);

                    Math::Float3 position0, normal0;
                    Math::Float3 position1, normal1;
                    NewtonMaterialGetContactPositionAndNormal(newtonMaterial, body0, position0.data, normal0.data);
                    NewtonMaterialGetContactPositionAndNormal(newtonMaterial, body1, position1.data, normal1.data);

                    uint32_t surfaceIndex0 = (newtonEntity0 ? newtonEntity0->getSurface(position0, normal0) : 0);
                    uint32_t surfaceIndex1 = (newtonEntity1 ? newtonEntity1->getSurface(position1, normal1) : 0);
                    const Surface &surface0 = processor->getSurface(surfaceIndex0);
                    const Surface &surface1 = processor->getSurface(surfaceIndex1);

                    processor->onCollision(entity0, position0, normal0, entity1);
                    processor->onCollision(entity1, position1, normal1, entity0);
                    if (surface0.ghost || surface1.ghost)
                    {
                        NewtonContactJointRemoveContact(contactJoint, newtonContact);
                    }
                    else
                    {
                        NewtonMaterialSetContactSoftness(newtonMaterial, ((surface0.softness + surface1.softness) * 0.5f));
                        NewtonMaterialSetContactElasticity(newtonMaterial, ((surface0.elasticity + surface1.elasticity) * 0.5f));
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface0.staticFriction, surface0.kineticFriction, 0);
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface1.staticFriction, surface1.kineticFriction, 1);
                    }
                }

                NewtonWorldCriticalSectionUnlock(newtonWorld);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Processor)
    }; // namespace Newton
}; // namespace Gek

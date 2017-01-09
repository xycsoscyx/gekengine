#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Editor.hpp"
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
        extern Newton::EntityPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, NewtonWorld *newtonWorld, Plugin::Entity *entity);
        extern EntityPtr createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Plugin::Entity *entity);

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
            , public Plugin::Processor
            , public Newton::World
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
                    wchar_t name[64];
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
            Plugin::Editor *editor = nullptr;

            NewtonWorld *newtonWorld = nullptr;
            NewtonCollision *newtonStaticScene = nullptr;
            NewtonBody *newtonStaticBody = nullptr;
            std::unordered_map<uint32_t, uint32_t> staticSurfaceMap;

            Math::Float3 gravity = Math::Float3(0.0f, -32.174f, 0.0f);
            concurrency::concurrent_vector<Surface> surfaceList;
            concurrency::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            concurrency::concurrent_unordered_map<Plugin::Entity *, Newton::EntityPtr> entityMap;
            concurrency::concurrent_unordered_map<std::size_t, NewtonCollision *> collisionMap;

        public:
            Processor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(core->getPopulation())
                , newtonWorld(NewtonCreate())
            {
                GEK_REQUIRE(core);
                GEK_REQUIRE(newtonWorld);

                NewtonSetSolverModel(newtonWorld, 1);
                NewtonSetFrictionModel(newtonWorld, 1);
                NewtonWorldSetUserData(newtonWorld, static_cast<Newton::World *>(this));

                NewtonWorldAddPreListener(newtonWorld, "__gek_pre_listener__", this, newtonWorldPreUpdate, nullptr);
                NewtonWorldAddPostListener(newtonWorld, "__gek_post_listener__", this, newtonWorldPostUpdate, nullptr);

                int defaultMaterialID = NewtonMaterialGetDefaultGroupID(newtonWorld);
#if NEWTON_MINOR_VERSION >= 14
                NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, newtonOnAABBOverlap, newtonOnContactFriction);
#else
                NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, nullptr, newtonOnAABBOverlap, newtonOnContactFriction);
#endif

                core->onInterface.connect<Processor, &Processor::onInterface>(this);
                population->onLoadBegin.connect<Processor, &Processor::onLoadBegin>(this);
                population->onLoadSucceeded.connect<Processor, &Processor::onLoadSucceeded>(this);
                population->onEntityDestroyed.connect<Processor, &Processor::onEntityDestroyed>(this);
                population->onUpdate[50].connect<Processor, &Processor::onUpdate>(this);
            }

            ~Processor(void)
            {
                if (editor)
                {
                    editor->onModified.disconnect<Processor, &Processor::onModified>(this);
                }

                population->onUpdate[50].disconnect<Processor, &Processor::onUpdate>(this);
                population->onEntityDestroyed.disconnect<Processor, &Processor::onEntityDestroyed>(this);
                population->onLoadSucceeded.disconnect<Processor, &Processor::onLoadSucceeded>(this);
                population->onLoadBegin.disconnect<Processor, &Processor::onLoadBegin>(this);
                core->onInterface.disconnect<Processor, &Processor::onInterface>(this);

                NewtonWaitForUpdateToFinish(newtonWorld);
                for (auto &collisionPair : collisionMap)
                {
                    if (collisionPair.second)
                    {
                        NewtonDestroyCollision(collisionPair.second);
                    }
                }

                if (newtonStaticScene)
                {
                    NewtonDestroyCollision(newtonStaticScene);
                }

                collisionMap.clear();
                entityMap.clear();
                surfaceList.clear();
                surfaceIndexMap.clear();
                newtonStaticBody = nullptr;
                NewtonDestroyAllBodies(newtonWorld);
                NewtonInvalidateCache(newtonWorld);
                NewtonDestroy(newtonWorld);
                GEK_REQUIRE(NewtonGetMemoryUsed() == 0);
            }

            uint32_t getStaticSceneSurface(const Math::Float3 &position, const Math::Float3 &normal)
            {
                dLong surfaceAttribute = 0;
                Math::Float3 collisionNormal;
                NewtonCollisionRayCast(newtonStaticScene, (position - normal).data, (position + normal).data, collisionNormal.data, &surfaceAttribute);
                if (surfaceAttribute > 0)
                {
                    return staticSurfaceMap[uint32_t(surfaceAttribute)];
                }

                return 0;
            }
            /*
            NewtonCollision *createCollision(const Components::Shape &shapeComponent)
            {
            GEK_REQUIRE(population);

            NewtonCollision *newtonCollision = nullptr;

            auto hash = GetHash(shapeComponent.type, shapeComponent.parameters);
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

            if (shapeComponent.type.compareNoCase(L"cube") == 0)
            {
            Math::Float3 size(Evaluator::Get<Math::Float3>(population->getShuntingYard(), shapeComponent.parameters));
            newtonCollision = NewtonCreateBox(newtonWorld, size.x, size.y, size.z, hash, Math::Float4x4::Identity.data);
            }
            else if (shapeComponent.type.compareNoCase(L"sphere") == 0)
            {
            float size = Evaluator::Get<float>(population->getShuntingYard(), shapeComponent.parameters);
            newtonCollision = NewtonCreateSphere(newtonWorld, size, hash, Math::Float4x4::Identity.data);
            }
            else if (shapeComponent.type.compareNoCase(L"cone") == 0)
            {
            Math::Float2 size(Evaluator::Get<Math::Float2>(population->getShuntingYard(), shapeComponent.parameters));
            newtonCollision = NewtonCreateCone(newtonWorld, size.x, size.y, hash, Math::Float4x4::Identity.data);
            }
            else if (shapeComponent.type.compareNoCase(L"capsule") == 0)
            {
            Math::Float2 size(Evaluator::Get<Math::Float2>(population->getShuntingYard(), shapeComponent.parameters));
            newtonCollision = NewtonCreateCapsule(newtonWorld, size.x, size.x, size.y, hash, Math::Float4x4::Identity.data);
            }
            else if (shapeComponent.type.compareNoCase(L"cylinder") == 0)
            {
            Math::Float2 size(Evaluator::Get<Math::Float2>(population->getShuntingYard(), shapeComponent.parameters));
            newtonCollision = NewtonCreateCylinder(newtonWorld, size.x, size.x, size.y, hash, Math::Float4x4::Identity.data);
            }
            else if (shapeComponent.type.compareNoCase(L"chamfer_cylinder") == 0)
            {
            Math::Float2 size(Evaluator::Get<Math::Float2>(population->getShuntingYard(), shapeComponent.parameters));
            newtonCollision = NewtonCreateChamferCylinder(newtonWorld, size.x, size.y, hash, Math::Float4x4::Identity.data);
            }

            if (newtonCollision == nullptr)
            {
            throw Newton::UnableToCreateCollision("Unable to create shape collision object");
            }

            collisionMap[hash] = newtonCollision;
            }

            return newtonCollision;
            }
            */
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

                    std::vector<uint8_t> buffer;
                    FileSystem::Load(getContext()->getRootFileName(L"data", L"models", modelComponent.name).withExtension(L".bin"), buffer);

                    Header *header = (Header *)buffer.data();
                    if (header->identifier != *(uint32_t *)"GEKX")
                    {
                        throw Newton::InvalidModelIdentifier("Unknown model file identifier encountered");
                    }

                    if (header->version != 2)
                    {
                        throw Newton::InvalidModelVersion("Unsupported model version encountered");
                    }

                    if (header->newtonVersion != NewtonWorldGetVersion())
                    {
                        throw Newton::InvalidModelVersion("Model created with different version of Newton Dynamics");
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
                        HullHeader *hullHeader = (HullHeader *)header;
                        DeSerializationData data(buffer, (uint8_t *)&hullHeader->serializationData[0]);
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, &data);
                    }
                    else if (header->type == 2)
                    {
                        TreeHeader *treeHeader = (TreeHeader *)header;
                        for (uint32_t materialIndex = 0; materialIndex < treeHeader->materialCount; ++materialIndex)
                        {
                            TreeHeader::Material &materialHeader = treeHeader->materialList[materialIndex];
                            staticSurfaceMap[materialIndex] = loadSurface(materialHeader.name);
                        }

                        DeSerializationData data(buffer, (uint8_t *)&treeHeader->materialList[treeHeader->materialCount]);
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, &data);
                    }
                    else
                    {
                        throw Newton::InvalidModelType("Unsupported model type encountered");
                    }

                    if (newtonCollision == nullptr)
                    {
                        throw Newton::UnableToCreateCollision("Unable to create model collision object");
                    }

                    NewtonCollisionSetMode(newtonCollision, true);
                    NewtonCollisionSetScale(newtonCollision, 1.0f, 1.0f, 1.0f);
                    NewtonCollisionSetMatrix(newtonCollision, Math::Float4x4::Identity.data);
                    collisionMap[hash] = newtonCollision;
                }

                return newtonCollision;
            }

            concurrency::critical_section criticalSection;
            void addEntity(Plugin::Entity *entity)
            {
                if (entity->hasComponents<Components::Transform, Components::Physical>())
                {
                    auto &transformComponent = entity->getComponent<Components::Transform>();
                    auto &physicalComponent = entity->getComponent<Components::Physical>();

                    concurrency::critical_section::scoped_lock lock(criticalSection);
                    if (entity->hasComponent<Components::Player>())
                    {
                        Newton::EntityPtr playerBody(createPlayerBody(core, population, newtonWorld, entity));
                        if (playerBody)
                        {
                            entityMap[entity] = playerBody;
                            NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                        }
                    }
                    else
                    {
                        NewtonCollision *newtonCollision = nullptr;
                        if (entity->hasComponent<Components::Model>())
                        {
                            const auto &modelComponent = entity->getComponent<Components::Model>();
                            newtonCollision = loadCollision(modelComponent);
                        }

                        if (newtonCollision)
                        {
                            if (physicalComponent.mass == 0.0f && loadingScene)
                            {
                                NewtonCollision *clonedCollision = NewtonCollisionCreateInstance(newtonCollision);
                                NewtonCollisionSetMatrix(clonedCollision, transformComponent.getMatrix().data);
                                NewtonSceneCollisionAddSubCollision(newtonStaticScene, clonedCollision);
                                NewtonDestroyCollision(clonedCollision);
                            }
                            else
                            {
                                Newton::EntityPtr rigidBody(createRigidBody(newtonWorld, newtonCollision, entity));
                                if (rigidBody)
                                {
                                    entityMap[entity] = rigidBody;
                                    NewtonBodySetTransformCallback(rigidBody->getNewtonBody(), newtonSetTransform);
                                }
                            }
                        }
                    }
                }
            }

            void removeEntity(Plugin::Entity *entity)
            {
                auto entitySearch = entityMap.find(entity);
                if (entitySearch != std::end(entityMap))
                {
                    NewtonDestroyBody(entitySearch->second->getNewtonBody());
                    entityMap.unsafe_erase(entitySearch);
                }
            }

            // Plugin::Processor
            void onInitialized(void)
            {
                core->listProcessors([&](Plugin::Processor *processor) -> void
                {
                    editor = dynamic_cast<Plugin::Editor *>(processor);
                    if (editor)
                    {
                        editor->onModified.connect<Processor, &Processor::onModified>(this);
                    }                    
                });
            }

            // Plugin::Editor Slots
            void onModified(Plugin::Entity *entity, const std::type_index &type)
            {
                if (type == typeid(Components::Transform))
                {
                    auto entitySearch = entityMap.find(entity);
                    if (entitySearch != std::end(entityMap))
                    {
                        auto &transformComponent = entity->getComponent<Components::Transform>();
                        NewtonBodySetMatrix(entitySearch->second->getNewtonBody(), transformComponent.getMatrix().data);
                    }
                }
            }

            // Plugin::Core Slots
            void onInterface(bool showCursor)
            {
            }

            // Plugin::Population Slots
            void onLoadBegin(const String &populationName)
            {
                NewtonWaitForUpdateToFinish(newtonWorld);
                for (auto &collisionPair : collisionMap)
                {
                    if (collisionPair.second)
                    {
                        NewtonDestroyCollision(collisionPair.second);
                    }
                }

                if (newtonStaticScene)
                {
                    NewtonDestroyCollision(newtonStaticScene);
                    newtonStaticScene = nullptr;
                }

                collisionMap.clear();
                entityMap.clear();
                surfaceList.clear();
                surfaceIndexMap.clear();
                newtonStaticBody = nullptr;
                NewtonDestroyAllBodies(newtonWorld);
                NewtonInvalidateCache(newtonWorld);

                surfaceList.push_back(Surface());
            }

            bool loadingScene = false;
            void onLoadSucceeded(const String &populationName)
            {
                loadingScene = true;
                newtonStaticScene = NewtonCreateSceneCollision(newtonWorld, 1);
                if (newtonStaticScene == nullptr)
                {
                    throw Newton::UnableToCreateCollision("Unable to create scene collision");
                }

                NewtonSceneCollisionBeginAddRemove(newtonStaticScene);
                population->listEntities([&](Plugin::Entity *entity, const wchar_t *) -> void
                {
                    addEntity(entity);
                });

                NewtonSceneCollisionEndAddRemove(newtonStaticScene);
                NewtonCollisionSetMode(newtonStaticScene, true);
                NewtonCollisionSetScale(newtonStaticScene, 1.0f, 1.0f, 1.0f);
                NewtonCollisionSetMatrix(newtonStaticScene, Math::Float4x4::Identity.data);

                newtonStaticBody = NewtonCreateDynamicBody(newtonWorld, newtonStaticScene, Math::Float4x4::Identity.data);
                NewtonBodySetCollidable(newtonStaticBody, true);
                loadingScene = false;
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
                if (!entity->hasComponents<Components::Transform, Components::Physical>())
                {
                    removeEntity(entity);
                }
            }

            void onUpdate(void)
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(newtonWorld);

                if (!core->isEditorActive() && population->getFrameTime() > 0.0f)
                {
                    float frameTime = population->getFrameTime();
                    static const float StepTime = (1.0f / 120.0f);
                    while (frameTime > 0.0f)
                    {
                        NewtonUpdate(newtonWorld, std::min(frameTime, StepTime));
                        frameTime -= StepTime;
                    };

                    NewtonWaitForUpdateToFinish(newtonWorld);
                }
            }

            // Newton::World
            Math::Float3 getGravity(const Math::Float3 &position)
            {
                const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);
                return Gravity;
            }

            uint32_t loadSurface(const wchar_t *surfaceName)
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
                    try
                    {
                        const JSON::Object materialNode = JSON::Load(getContext()->getRootFileName(L"data", L"materials", surfaceName).withExtension(L".json"));

                        auto &surfaceNode = materialNode.get(L"surface");
                        if (surfaceNode.is_object())
                        {
                            Surface surface;
                            surface.ghost = surfaceNode.get(L"ghost", surface.ghost).as_bool();
                            surface.staticFriction = surfaceNode.get(L"static_friction", surface.staticFriction).as<float>();
                            surface.kineticFriction = surfaceNode.get(L"kinetic_friction", surface.kineticFriction).as<float>();
                            surface.elasticity = surfaceNode.get(L"elasticity", surface.elasticity).as<float>();
                            surface.softness = surfaceNode.get(L"softness", surface.softness).as<float>();

                            surfaceIndex = surfaceList.size();
                            surfaceList.push_back(surface);
                            surfaceIndexMap[hash] = surfaceIndex;
                        }
                    }
                    catch (const std::exception &)
                    {
                    };
                }

                return surfaceIndex;
            }

            const Surface & getSurface(uint32_t surfaceIndex) const
            {
                return surfaceList[surfaceIndex];
            }

            // Processor
            static void newtonWorldPreUpdate(const NewtonWorld* const world, void* const userData, float frameTime)
            {
                std::map<Newton::Entity *, float> updateMap;
                Processor *processor = static_cast<Processor *>(userData);
                for (auto &entityPair : processor->entityMap)
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
                for (auto &entityPair : processor->entityMap)
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
                Newton::World *processorBase = static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld));
                Processor *processor = dynamic_cast<Processor *>(processorBase);

                Newton::Entity *newtonEntity0 = static_cast<Newton::Entity *>(NewtonBodyGetUserData(body0));
                Newton::Entity *newtonEntity1 = static_cast<Newton::Entity *>(NewtonBodyGetUserData(body1));
                Plugin::Entity *entity0 = (newtonEntity0 ? newtonEntity0->getEntity() : nullptr);
                Plugin::Entity *entity1 = (newtonEntity1 ? newtonEntity1->getEntity() : nullptr);

                NewtonWorldCriticalSectionLock(newtonWorld, threadHandle);
                for (void* newtonContact = NewtonContactJointGetFirstContact(contactJoint); newtonContact; newtonContact = NewtonContactJointGetNextContact(contactJoint, newtonContact))
                {
                    NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                    Math::Float3 position, normal;
                    NewtonMaterialGetContactPositionAndNormal(newtonMaterial, body0, position.data, normal.data);
                    uint32_t surfaceIndex0 = (newtonEntity0 ? newtonEntity0->getSurface(position, normal) : processor->getStaticSceneSurface(position, normal));
                    uint32_t surfaceIndex1 = (newtonEntity1 ? newtonEntity1->getSurface(position, normal) : processor->getStaticSceneSurface(position, normal));
                    const Surface &surface0 = processor->getSurface(surfaceIndex0);
                    const Surface &surface1 = processor->getSurface(surfaceIndex1);

                    processor->onCollision.emit(entity0, entity1, position, normal);
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

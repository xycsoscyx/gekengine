#include "GEK\Math\Common.hpp"
#include "GEK\Math\Float4x4.hpp"
#include "GEK\Shapes\AlignedBox.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\Evaluator.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\Hash.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Newton\Base.hpp"
#include "GEK\Model\Base.hpp"
#include "GEK\Shape\Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>

#include <Newton.h>

namespace Gek
{
    namespace Newton
    {
        extern EntityPtr createPlayerBody(Plugin::Core *core, NewtonWorld *newtonWorld, Plugin::Entity *entity);
        extern EntityPtr createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Plugin::Entity *entity);

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
            , public Plugin::Processor
            , public Newton::World
        {
        public:
            struct Header
            {
                uint32_t identifier;
                uint16_t type;
                uint16_t version;
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
            Plugin::Core *core;
            Plugin::Population *population;
            uint32_t updateHandle;

            NewtonWorld *newtonWorld;
            NewtonCollision *newtonStaticScene;
            NewtonBody *newtonStaticBody;
            std::unordered_map<uint32_t, uint32_t> staticSurfaceMap;

            Math::Float3 gravity;
            concurrency::concurrent_vector<Surface> surfaceList;
            concurrency::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            concurrency::concurrent_unordered_map<Plugin::Entity *, Newton::EntityPtr> entityMap;
            concurrency::concurrent_unordered_map<std::size_t, NewtonCollision *> collisionMap;

        public:
            Processor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(core->getPopulation())
                , updateHandle(0)
                , newtonWorld(NewtonCreate())
                , newtonStaticScene(nullptr)
                , newtonStaticBody(nullptr)
                , gravity(0.0f, -32.174f, 0.0f)
            {
                GEK_REQUIRE(core);
                GEK_REQUIRE(newtonWorld);

                NewtonWorldSetUserData(newtonWorld, static_cast<Newton::World *>(this));

                NewtonWorldAddPreListener(newtonWorld, "__gek_pre_listener__", this, newtonWorldPreUpdate, nullptr);
                NewtonWorldAddPostListener(newtonWorld, "__gek_post_listener__", this, newtonWorldPostUpdate, nullptr);

                int defaultMaterialID = NewtonMaterialGetDefaultGroupID(newtonWorld);
                NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, newtonOnAABBOverlap, newtonOnContactFriction);

                population->onLoadBegin.connect<Processor, &Processor::onLoadBegin>(this);
                population->onLoadSucceeded.connect<Processor, &Processor::onLoadSucceeded>(this);
                population->onEntityDestroyed.connect<Processor, &Processor::onEntityDestroyed>(this);
                population->onUpdate[50].connect<Processor, &Processor::onUpdate>(this);
            }

            ~Processor(void)
            {
                population->onUpdate[50].disconnect<Processor, &Processor::onUpdate>(this);
                population->onEntityDestroyed.disconnect<Processor, &Processor::onEntityDestroyed>(this);
                population->onLoadSucceeded.disconnect<Processor, &Processor::onLoadSucceeded>(this);
                population->onLoadBegin.disconnect<Processor, &Processor::onLoadBegin>(this);

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

            void onLoadSucceeded(const String &populationName)
            {
                newtonStaticScene = NewtonCreateSceneCollision(newtonWorld, 1);
                if (newtonStaticScene == nullptr)
                {
                    throw Newton::UnableToCreateCollision();
                }

                NewtonSceneCollisionBeginAddRemove(newtonStaticScene);
                population->listEntities<Components::Physical, Components::Transform>([&](Plugin::Entity *entity, const wchar_t *) -> void
                {
                    const auto &transformComponent = entity->getComponent<Components::Transform>();
                    const auto &physicalComponent = entity->getComponent<Components::Physical>();
                    if (entity->hasComponent<Components::Player>())
                    {
                        Newton::EntityPtr playerBody(createPlayerBody(core, newtonWorld, entity));
                        if (playerBody)
                        {
                            entityMap[entity] = playerBody;
                            NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                        }
                    }
                    else
                    {
                        NewtonCollision *newtonCollision = nullptr;
                        if (entity->hasComponents<Components::Model>())
                        {
                            const auto &modelComponent = entity->getComponent<Components::Model>();
                            newtonCollision = loadCollision(modelComponent);
                        }
                        else if (entity->hasComponents<Components::Shape>())
                        {
                            const auto &shapeComponent = entity->getComponent<Components::Shape>();
                            newtonCollision = createCollision(shapeComponent);
                        }

                        if (newtonCollision)
                        {
                            if (physicalComponent.mass == 0.0f)
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
                });

                NewtonSceneCollisionEndAddRemove(newtonStaticScene);
                newtonStaticBody = NewtonCreateDynamicBody(newtonWorld, newtonStaticScene, Math::Float4x4::Identity.data);
            }

            void onEntityDestroyed(Plugin::Entity *entity)
            {
                GEK_REQUIRE(entity);

                auto entitySearch = entityMap.find(entity);
                if (entitySearch != entityMap.end())
                {
                    entityMap.unsafe_erase(entitySearch);
                }
            }

            void onUpdate(void)
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(newtonWorld);

                NewtonUpdateAsync(newtonWorld, population->getFrameTime());
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

                auto hash = getHash(surfaceName);
                auto surfaceSearch = surfaceIndexMap.find(hash);
                if (surfaceSearch != surfaceIndexMap.end())
                {
                    surfaceIndex = surfaceSearch->second;
                }
                else
                {
                    surfaceIndexMap[hash] = 0;
                    try
                    {
                        const Xml::Node materialNode(Xml::load(getContext()->getFileName(L"data\\materials", surfaceName).append(L".xml"), L"material"));
                        auto &surfaceNode = materialNode.getChild(L"surface");
                        if (surfaceNode.valid)
                        {
                            Surface surface;
                            surface.ghost = surfaceNode.getValue(L"ghost", surface.ghost);
                            surface.staticFriction = surfaceNode.getValue(L"static_friction", surface.staticFriction);
                            surface.kineticFriction = surfaceNode.getValue(L"kinetic_friction", surface.kineticFriction);
                            surface.elasticity = surfaceNode.getValue(L"elasticity", surface.elasticity);
                            surface.softness = surfaceNode.getValue(L"softness", surface.softness);

                            surfaceIndex = surfaceList.size();
                            surfaceList.push_back(surface);
                            surfaceIndexMap[hash] = surfaceIndex;
                        }
                    }
                    catch (const Xml::Exception &)
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

        private:
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

            NewtonCollision *createCollision(const Components::Shape &shapeComponent)
            {
                GEK_REQUIRE(population);

                NewtonCollision *newtonCollision = nullptr;

                auto hash = getHash(shapeComponent.type, shapeComponent.parameters);
                auto collisionSearch = collisionMap.find(hash);
                if (collisionSearch != collisionMap.end())
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
                        Math::Float3 size(Evaluator::get<Math::Float3>(shapeComponent.parameters));
                        newtonCollision = NewtonCreateBox(newtonWorld, size.x, size.y, size.z, hash, Math::Float4x4::Identity.data);
                    }
                    else if (shapeComponent.type.compareNoCase(L"sphere") == 0)
                    {
                        float size = Evaluator::get<float>(shapeComponent.parameters);
                        newtonCollision = NewtonCreateSphere(newtonWorld, size, hash, Math::Float4x4::Identity.data);
                    }
                    else if (shapeComponent.type.compareNoCase(L"cone") == 0)
                    {
                        Math::Float2 size(Evaluator::get<Math::Float2>(shapeComponent.parameters));
                        newtonCollision = NewtonCreateCone(newtonWorld, size.x, size.y, hash, Math::Float4x4::Identity.data);
                    }
                    else if (shapeComponent.type.compareNoCase(L"capsule") == 0)
                    {
                        Math::Float2 size(Evaluator::get<Math::Float2>(shapeComponent.parameters));
                        newtonCollision = NewtonCreateCapsule(newtonWorld, size.x, size.x, size.y, hash, Math::Float4x4::Identity.data);
                    }
                    else if (shapeComponent.type.compareNoCase(L"cylinder") == 0)
                    {
                        Math::Float2 size(Evaluator::get<Math::Float2>(shapeComponent.parameters));
                        newtonCollision = NewtonCreateCylinder(newtonWorld, size.x, size.x, size.y, hash, Math::Float4x4::Identity.data);
                    }
                    /*
                    else if (shapeComponent.type.compareNoCase(L"tapered_capsule") == 0)
                    {
                    Math::Float3 size(Evaluator::get<Math::Float3>(shapeComponent.parameters));
                    newtonCollision = NewtonCreateTaperedCapsule(newtonWorld, size.x, size.y, size.z, hash, Math::Float4x4::Identity.data);
                    }
                    else if (shapeComponent.type.compareNoCase(L"tapered_cylinder") == 0)
                    {
                    Math::Float3 size(Evaluator::get<Math::Float3>(shapeComponent.parameters));
                    newtonCollision = NewtonCreateTaperedCylinder(newtonWorld, size.x, size.y, size.z, hash, Math::Float4x4::Identity.data);
                    }
                    */
                    else if (shapeComponent.type.compareNoCase(L"chamfer_cylinder") == 0)
                    {
                        Math::Float2 size(Evaluator::get<Math::Float2>(shapeComponent.parameters));
                        newtonCollision = NewtonCreateChamferCylinder(newtonWorld, size.x, size.y, hash, Math::Float4x4::Identity.data);
                    }

                    if (newtonCollision == nullptr)
                    {
                        throw Newton::UnableToCreateCollision();
                    }

                    collisionMap[hash] = newtonCollision;
                }

                return newtonCollision;
            }

            NewtonCollision *loadCollision(const Components::Model &modelComponent)
            {
                NewtonCollision *newtonCollision = nullptr;

                auto hash = getHash(modelComponent.name);
                auto collisionSearch = collisionMap.find(hash);
                if (collisionSearch != collisionMap.end())
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
					FileSystem::load(getContext()->getFileName(L"data\\models", modelComponent.name).append(L".bin"), buffer);

                    Header *header = (Header *)buffer.data();
                    if (header->identifier != *(uint32_t *)"GEKX")
                    {
                        throw InvalidModelIdentifier();
                    }

					if (header->version != 1)
                    {
                        throw Newton::InvalidModelVersion();
                    }

					struct DeSerializationData
					{
						std::vector<uint8_t> &buffer;
						std::size_t offset;

						DeSerializationData(std::vector<uint8_t> &buffer, uint8_t *end)
							: buffer(buffer)
							, offset(end - &buffer.at(0))
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
                        for (uint32_t materialIndex = 0; materialIndex < treeHeader->materialCount; materialIndex++)
                        {
                            TreeHeader::Material &materialHeader = treeHeader->materialList[materialIndex];
                            staticSurfaceMap[materialIndex] = loadSurface(materialHeader.name);
                        }

						DeSerializationData data(buffer, (uint8_t *)&treeHeader->materialList[treeHeader->materialCount]);
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, &data);
                    }
                    else
                    {
                        throw Newton::InvalidModelType();
                    }

                    if (newtonCollision == nullptr)
                    {
                        throw Newton::UnableToCreateCollision();
                    }

                    collisionMap[hash] = newtonCollision;
                }

                return newtonCollision;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Processor)
    }; // namespace Newton
}; // namespace Gek

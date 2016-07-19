#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\StaticBody.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\Entity.h"
#include "GEK\Newton\World.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include <Newton.h>
#include <memory>
#include <map>
#include <set>

static void deSerializeCollision(void* const serializeHandle, void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fread(buffer, 1, size, file);
}

namespace Gek
{
    extern Newton::EntityPtr createPlayerBody(Plugin::Core *core, NewtonWorld *newtonWorld, Plugin::Entity *entity);
    extern Newton::EntityPtr createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Plugin::Entity *entity);

    GEK_CONTEXT_USER(NewtonProcessor, Plugin::Core *)
        , public ObservableMixin<Newton::WorldObserver>
        , public Plugin::PopulationObserver
        , public Plugin::Processor
        , public Newton::World
    {
    public:
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
        std::vector<Surface> surfaceList;
        std::unordered_map<std::size_t, uint32_t> surfaceIndexMap;
        std::unordered_map<Plugin::Entity *, Newton::EntityPtr> entityMap;
        std::unordered_map<std::size_t, NewtonCollision *> collisionMap;

    public:
        NewtonProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
            , updateHandle(0)
            , newtonWorld(nullptr)
            , newtonStaticScene(nullptr)
            , newtonStaticBody(nullptr)
            , gravity(0.0f, -32.174f, 0.0f)
        {
            updateHandle = population->setUpdatePriority(this, 50);
            population->addObserver((Plugin::PopulationObserver *)this);
        }

        ~NewtonProcessor(void)
        {
            onFree();
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeObserver((Plugin::PopulationObserver *)this);
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
            GEK_REQUIRE(surfaceName);

            uint32_t surfaceIndex = 0;
            std::size_t hash = std::hash<String>()(surfaceName);
            auto surfaceSearch = surfaceIndexMap.find(hash);
            if (surfaceSearch != surfaceIndexMap.end())
            {
                surfaceIndex = (*surfaceSearch).second;
            }
            else
            {
                auto &surfaceIndex = surfaceIndexMap[hash] = 0;

                XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\materials\\%v.xml", surfaceName)));
                XmlNodePtr materialNode(document->getRoot(L"material"));
                XmlNodePtr surfaceNode(materialNode->firstChildElement(L"surface"));
                if (surfaceNode->isValid())
                {
                    Surface surface;
                    surface.ghost = surfaceNode->getAttribute(L"ghost");
                    if (surfaceNode->hasAttribute(L"staticfriction"))
                    {
                        surface.staticFriction = surfaceNode->getAttribute(L"staticfriction");
                    }

                    if (surfaceNode->hasAttribute(L"kineticfriction"))
                    {
                        surface.kineticFriction = surfaceNode->getAttribute(L"kineticfriction");
                    }

                    if (surfaceNode->hasAttribute(L"elasticity"))
                    {
                        surface.elasticity = surfaceNode->getAttribute(L"elasticity");
                    }

                    if (surfaceNode->hasAttribute(L"softness"))
                    {
                        surface.softness = surfaceNode->getAttribute(L"softness");
                    }

                    surfaceIndex = surfaceList.size();
                    surfaceList.push_back(surface);
                }
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
            NewtonProcessor *processor = static_cast<NewtonProcessor *>(userData);
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
            NewtonProcessor *processor = static_cast<NewtonProcessor *>(userData);
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
            NewtonProcessor *processor = dynamic_cast<NewtonProcessor *>(processorBase);

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

                processor->ObservableMixin::sendEvent(Event(std::bind(&Newton::WorldObserver::onCollision, std::placeholders::_1, entity0, entity1, position, normal)));
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

        // Plugin::PopulationObserver
        void onLoadBegin(void)
        {
            surfaceList.push_back(Surface());

            newtonWorld = NewtonCreate();
            NewtonWorldSetUserData(newtonWorld, static_cast<Newton::World *>(this));

            NewtonWorldAddPreListener(newtonWorld, "__gek_pre_listener__", this, newtonWorldPreUpdate, nullptr);
            NewtonWorldAddPostListener(newtonWorld, "__gek_post_listener__", this, newtonWorldPostUpdate, nullptr);

            int defaultMaterialID = NewtonMaterialGetDefaultGroupID(newtonWorld);
            NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, newtonOnAABBOverlap, newtonOnContactFriction);

            newtonStaticScene = NewtonCreateSceneCollision(newtonWorld, 1);
            if (newtonStaticScene)
            {
                NewtonSceneCollisionBeginAddRemove(newtonStaticScene);
            }
        }

        void onLoadSucceeded(void)
        {
            if (newtonStaticScene)
            {
                NewtonSceneCollisionEndAddRemove(newtonStaticScene);
                newtonStaticBody = NewtonCreateDynamicBody(newtonWorld, newtonStaticScene, Math::Float4x4::Identity.data);
                NewtonBodySetMassProperties(newtonStaticBody, 0.0f, newtonStaticScene);
            }
        }

        void onLoadFailed(void)
        {
            if (newtonStaticScene)
            {
                NewtonSceneCollisionEndAddRemove(newtonStaticScene);
            }

            onFree();
        }

        void onFree(void)
        {
            if (newtonWorld)
            {
                NewtonWaitForUpdateToFinish(newtonWorld);
                //NewtonSerializeToFile(newtonWorld, CW2A(FileSystem::expandPath(L"$root\\data\\newton.bin")), nullptr, nullptr);
            }

            for (auto &collisionPair : collisionMap)
            {
                NewtonDestroyCollision(collisionPair.second);
            }

            collisionMap.clear();
            entityMap.clear();
            surfaceList.clear();
            surfaceIndexMap.clear();

            if (newtonStaticScene)
            {
                NewtonDestroyCollision(newtonStaticScene);
                newtonStaticScene = nullptr;
            }

            newtonStaticBody = nullptr;
            if (newtonWorld)
            {
                NewtonDestroyAllBodies(newtonWorld);
                NewtonInvalidateCache(newtonWorld);
                NewtonDestroy(newtonWorld);
                newtonWorld = nullptr;
            }

            GEK_REQUIRE(NewtonGetMemoryUsed() == 0);
        }

        void onEntityCreated(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            if (entity->hasComponents<Components::Transform>())
            {
                auto &transformComponent = entity->getComponent<Components::Transform>();
                if (entity->hasComponent<Components::StaticBody>())
                {
                    auto &staticBodyComponent = entity->getComponent<Components::StaticBody>();
                    NewtonCollision *newtonCollision = loadCollision(entity, staticBodyComponent.shape);
                    if (newtonCollision != nullptr)
                    {
                        NewtonCollision *clonedCollision = NewtonCollisionCreateInstance(newtonCollision);
                        NewtonCollisionSetMatrix(clonedCollision, transformComponent.getMatrix().data);
                        NewtonSceneCollisionAddSubCollision(newtonStaticScene, clonedCollision);
                        NewtonDestroyCollision(clonedCollision);
                    }
                }
                else if (entity->hasComponents<Components::Mass>())
                {
                    auto &massComponent = entity->getComponent<Components::Mass>();
                    if (entity->hasComponent<Components::RigidBody>())
                    {
                        auto &rigidBodyComponent = entity->getComponent<Components::RigidBody>();
                        NewtonCollision *newtonCollision = loadCollision(entity, rigidBodyComponent.shape);
                        if (newtonCollision != nullptr)
                        {
                            Newton::EntityPtr rigidBody(createRigidBody(newtonWorld, newtonCollision, entity));
                            if (rigidBody)
                            {
                                entityMap[entity] = rigidBody;
                                NewtonBodySetTransformCallback(rigidBody->getNewtonBody(), newtonSetTransform);
                            }
                        }
                    }
                    else if (entity->hasComponent<Components::PlayerBody>())
                    {
                        auto &playerBodyComponent = entity->getComponent<Components::PlayerBody>();
                        Newton::EntityPtr playerBody(createPlayerBody(core, newtonWorld, entity));
                        if (playerBody)
                        {
                            entityMap[entity] = playerBody;
                            NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                        }
                    }
                }
            }
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entitySearch = entityMap.find(entity);
            if (entitySearch != entityMap.end())
            {
                entityMap.erase(entitySearch);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));
            GEK_REQUIRE(population);

            if (!isIdle && newtonWorld)
            {
                NewtonUpdateAsync(newtonWorld, population->getFrameTime());
            }
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

        NewtonCollision *createCollision(Plugin::Entity *entity, const String &shape)
        {
            GEK_REQUIRE(population);

            NewtonCollision *newtonCollision = nullptr;
            std::size_t collisionHash = std::hash<String>()(shape);
            auto collisionSearch = collisionMap.find(collisionHash);
            if (collisionSearch != collisionMap.end())
            {
                if ((*collisionSearch).second)
                {
                    newtonCollision = (*collisionSearch).second;
                }
            }
            else
            {
                collisionMap[collisionHash] = nullptr;

                std::vector<String> parameters(shape.split(L'|'));
                GEK_CHECK_CONDITION(parameters.size() != 2, Trace::Exception, "Invalid parameters passed for shape: %v", shape);

                if (parameters[0].compareNoCase(L"*cube") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters[1]));
                    newtonCollision = NewtonCreateBox(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*sphere") == 0)
                {
                    float size = Evaluator::get<float>(parameters[1]);
                    newtonCollision = NewtonCreateSphere(newtonWorld, size, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*cone") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateCone(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*capsule") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateCapsule(newtonWorld, size.x, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateCylinder(newtonWorld, size.x, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
/*
                else if (parameters[0].compareNoCase(L"*tapered_capsule") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters[1]));
                    newtonCollision = NewtonCreateTaperedCapsule(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*tapered_cylinder") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters[1]));
                    newtonCollision = NewtonCreateTaperedCylinder(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
*/
                else if (parameters[0].compareNoCase(L"*chamfer_cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateChamferCylinder(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }

                GEK_CHECK_CONDITION(newtonCollision == nullptr, Trace::Exception, "Unable to create newton collision shape: %v", shape);
                collisionMap[collisionHash] = newtonCollision;
            }

            return newtonCollision;
        }

        NewtonCollision *loadCollision(Plugin::Entity *entity, const String &shape)
        {
            NewtonCollision *newtonCollision = nullptr;
            if (shape.at(0) == L'*')
            {
                newtonCollision = createCollision(entity, shape);
            }
            else
            {
                std::size_t shapeHash = std::hash<String>()(shape);
                auto collisionSearch = collisionMap.find(shapeHash);
                if (collisionSearch != collisionMap.end())
                {
                    if ((*collisionSearch).second)
                    {
                        newtonCollision = (*collisionSearch).second;
                    }
                }
                else
                {
                    collisionMap[shapeHash] = nullptr;

                    String fileName(FileSystem::expandPath(String(L"$root\\data\\models\\%v.bin", shape)));

                    FILE *file = nullptr;
                    _wfopen_s(&file, fileName, L"rb");
                    GEK_CHECK_CONDITION(file == nullptr, FileSystem::Exception, "Unable to load collision model: %v", fileName);

                    uint32_t gekIdentifier = 0;
                    fread(&gekIdentifier, sizeof(uint32_t), 1, file);
                    GEK_CHECK_CONDITION(gekIdentifier != *(uint32_t *)"GEKX", Trace::Exception, "Invalid model idetifier found: %v", gekIdentifier);

                    uint16_t gekModelType = 0;
                    fread(&gekModelType, sizeof(uint16_t), 1, file);

                    uint16_t gekModelVersion = 0;
                    fread(&gekModelVersion, sizeof(uint16_t), 1, file);

                    if (gekModelType == 1 && gekModelVersion == 0)
                    {
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, file);
                    }
                    else if (gekModelType == 2 && gekModelVersion == 0)
                    {
                        uint32_t materialCount = 0;
                        fread(&materialCount, sizeof(uint32_t), 1, file);
                        for (uint32_t materialIndex = 0; materialIndex < materialCount; materialIndex++)
                        {
                            String materialName;
                            wchar_t letter = 0;
                            do
                            {
                                fread(&letter, sizeof(wchar_t), 1, file);
                                materialName += letter;
                            } while (letter != 0);

                            staticSurfaceMap[materialIndex] = loadSurface(materialName);
                        }

                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, file);
                    }
                    else
                    {
                        GEK_THROW_EXCEPTION(Trace::Exception, "Invalid model format/version specified: %v, %v", gekModelType, gekModelVersion);
                    }

                    fclose(file);
                    GEK_CHECK_CONDITION(newtonCollision == nullptr, Trace::Exception, "Unable to create newton collision shape: %v", shape);
                    collisionMap[shapeHash] = newtonCollision;
                }
            }

            return newtonCollision;
        }
    };

    GEK_REGISTER_CONTEXT_USER(NewtonProcessor)
}; // namespace Gek

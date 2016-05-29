#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\StaticBody.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\NewtonProcessor.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include <Newton.h>
#include <memory>
#include <map>
#include <set>

#pragma comment(lib, "newton.lib")

static void deSerializeCollision(void* const serializeHandle, void* const buffer, int size)
{
    FILE *file = (FILE *)serializeHandle;
    fread(buffer, 1, size, file);
}

namespace Gek
{
    extern NewtonEntityPtr createPlayerBody(EngineContext *engine, NewtonWorld *newtonWorld, Entity *entity, PlayerBodyComponent &playerBodyComponent, TransformComponent &transformComponent, MassComponent &massComponent);
    extern NewtonEntityPtr createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Entity *entity, TransformComponent &transformComponent, MassComponent &massComponent);

    class NewtonProcessorImplementation
        : public ContextRegistration<NewtonProcessorImplementation, EngineContext *>
        , virtual public ObservableMixin<NewtonObserver>
        , public PopulationObserver
        , public Processor
        , public NewtonProcessor
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
            UINT32 firstVertex;
            UINT32 firstIndex;
            UINT32 indexCount;
        };

    private:
        EngineContext *engine;
        Population *population;
        UINT32 updateHandle;

        NewtonWorld *newtonWorld;
        NewtonCollision *newtonStaticScene;
        NewtonBody *newtonStaticBody;
        std::unordered_map<UINT32, UINT32> staticSurfaceMap;

        Math::Float3 gravity;
        std::vector<Surface> surfaceList;
        std::unordered_map<std::size_t, UINT32> surfaceIndexList;
        std::unordered_map<Entity *, NewtonEntityPtr> entityMap;
        std::unordered_map<std::size_t, NewtonCollision *> collisionList;

    public:
        NewtonProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , engine(engine)
            , population(engine->getPopulation())
            , updateHandle(0)
            , newtonWorld(nullptr)
            , newtonStaticScene(nullptr)
            , newtonStaticBody(nullptr)
            , gravity(0.0f, -32.174f, 0.0f)
        {
            updateHandle = population->setUpdatePriority(this, 50);
            population->addObserver((PopulationObserver *)this);
        }

        ~NewtonProcessorImplementation(void)
        {
            onFree();
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeObserver((PopulationObserver *)this);
            }
        }

        // NewtonProcessor
        Math::Float3 getGravity(const Math::Float3 &position)
        {
            const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);
            return Gravity;
        }

        UINT32 loadSurface(const wchar_t *fileName)
        {
            GEK_REQUIRE(fileName);

            UINT32 surfaceIndex = 0;
            std::size_t fileNameHash = std::hash<wstring>()(fileName);
            auto surfaceIterator = surfaceIndexList.find(fileNameHash);
            if (surfaceIterator != surfaceIndexList.end())
            {
                surfaceIndex = (*surfaceIterator).second;
            }
            else
            {
                HRESULT resultValue = E_FAIL;

                auto &surfaceIndex = surfaceIndexList[fileNameHash] = 0;

                Gek::XmlDocumentPtr document(XmlDocument::load(Gek::String::format(L"$root\\data\\materials\\%v.xml", fileName)));
                Gek::XmlNodePtr materialNode = document->getRoot(L"material");
                Gek::XmlNodePtr surfaceNode = materialNode->firstChildElement(L"surface");
                if (surfaceNode->isValid())
                {
                    Surface surface;
                    surface.ghost = String::to<bool>(surfaceNode->getAttribute(L"ghost"));
                    if (surfaceNode->hasAttribute(L"staticfriction"))
                    {
                        surface.staticFriction = Gek::String::to<float>(surfaceNode->getAttribute(L"staticfriction"));
                    }

                    if (surfaceNode->hasAttribute(L"kineticfriction"))
                    {
                        surface.kineticFriction = Gek::String::to<float>(surfaceNode->getAttribute(L"kineticfriction"));
                    }

                    if (surfaceNode->hasAttribute(L"elasticity"))
                    {
                        surface.elasticity = Gek::String::to<float>(surfaceNode->getAttribute(L"elasticity"));
                    }

                    if (surfaceNode->hasAttribute(L"softness"))
                    {
                        surface.softness = Gek::String::to<float>(surfaceNode->getAttribute(L"softness"));
                    }

                    surfaceIndex = surfaceList.size();
                    surfaceList.push_back(surface);
                }
            }

            return surfaceIndex;
        }

        const Surface & getSurface(UINT32 surfaceIndex) const
        {
            return surfaceList[surfaceIndex];
        }

        // Processor
        static void newtonPreUpdateTask(NewtonWorld* const world, void* const userData, int threadIndex)
        {
            auto updatePair = (std::pair<NewtonEntity *, float> *)userData;
            updatePair->first->onPreUpdate(updatePair->second, threadIndex);
        }

        void onPreUpdate(dFloat frameTime)
        {
            for (auto &entityPair : entityMap)
            {
                NewtonDispachThreadJob(newtonWorld, newtonPreUpdateTask, &std::make_pair(entityPair.second, frameTime));
            }

            NewtonSyncThreadJobs(newtonWorld);
        }

        static void newtonWorldPreUpdate(const NewtonWorld* const world, void* const listenerUserData, dFloat frameTime)
        {
            NewtonProcessorImplementation *processor = static_cast<NewtonProcessorImplementation *>(listenerUserData);
            processor->onPreUpdate(frameTime);
        }

        static void newtonPostUpdateTask(NewtonWorld* const world, void* const userData, int threadIndex)
        {
            auto updatePair = *(std::pair<NewtonEntity *, float> *)userData;
            updatePair.first->onPostUpdate(updatePair.second, threadIndex);
        }

        void onPostUpdate(dFloat frameTime)
        {
            for (auto &entityPair : entityMap)
            {
                NewtonDispachThreadJob(newtonWorld, newtonPostUpdateTask, &std::make_pair(entityPair.second, frameTime));
            }

            NewtonSyncThreadJobs(newtonWorld);
        }

        static void newtonWorldPostUpdate(const NewtonWorld* const world, void* const listenerUserData, dFloat frameTime)
        {
            NewtonProcessorImplementation *processor = static_cast<NewtonProcessorImplementation *>(listenerUserData);
            processor->onPostUpdate(frameTime);
        }

        static void newtonSetTransform(const NewtonBody* const body, const dFloat* const matrixData, int threadHandle)
        {
            NewtonEntity *newtonEntity = static_cast<NewtonEntity *>(NewtonBodyGetUserData(body));
            newtonEntity->onSetTransform(matrixData, threadHandle);
        }

        static int newtonOnAABBOverlap(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int threadHandle)
        {
            return 1;
        }

        static void newtonOnContactFriction(const NewtonJoint* contactJoint, dFloat frameTime, int threadHandle)
        {
            const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
            const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);

            NewtonWorld *newtonWorld = NewtonBodyGetWorld(body0);
            NewtonProcessor *processorBase = static_cast<NewtonProcessor *>(NewtonWorldGetUserData(newtonWorld));
            NewtonProcessorImplementation *processor = dynamic_cast<NewtonProcessorImplementation *>(processorBase);
            processor->onContactFriction(contactJoint, frameTime, threadHandle);
        }

        void onContactFriction(const NewtonJoint* contactJoint, dFloat frameTime, int threadHandle)
        {
            const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
            const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);
            NewtonEntity *newtonEntity0 = static_cast<NewtonEntity *>(NewtonBodyGetUserData(body0));
            NewtonEntity *newtonEntity1 = static_cast<NewtonEntity *>(NewtonBodyGetUserData(body1));
            Entity *entity0 = (newtonEntity0 ? newtonEntity0->getEntity() : nullptr);
            Entity *entity1 = (newtonEntity1 ? newtonEntity1->getEntity() : nullptr);

            NewtonWorldCriticalSectionLock(newtonWorld, threadHandle);
            for (void* newtonContact = NewtonContactJointGetFirstContact(contactJoint); newtonContact; newtonContact = NewtonContactJointGetNextContact(contactJoint, newtonContact))
            {
                NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                Math::Float3 position, normal;
                NewtonMaterialGetContactPositionAndNormal(newtonMaterial, body0, position.data, normal.data);
                UINT32 surfaceIndex0 = (newtonEntity0 ? newtonEntity0->getSurface(position, normal) : getStaticSceneSurface(position, normal));
                UINT32 surfaceIndex1 = (newtonEntity1 ? newtonEntity1->getSurface(position, normal) : getStaticSceneSurface(position, normal));
                const Surface &surface0 = getSurface(surfaceIndex0);
                const Surface &surface1 = getSurface(surfaceIndex1);

                ObservableMixin::sendEvent(Event(std::bind(&NewtonObserver::onCollision, std::placeholders::_1, entity0, entity1, position, normal)));
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

        // PopulationObserver
        void onLoadBegin(void)
        {
            surfaceList.push_back(Surface());

            newtonWorld = NewtonCreate();
            NewtonWorldSetUserData(newtonWorld, static_cast<NewtonProcessor *>(this));

            NewtonWorldAddPreListener(newtonWorld, "__gek_pre_listener__", this, newtonWorldPreUpdate, nullptr);
            NewtonWorldAddPostListener(newtonWorld, "__gek_post_listener__", this, newtonWorldPostUpdate, nullptr);

            int defaultMaterialID = NewtonMaterialGetDefaultGroupID(newtonWorld);
            NewtonMaterialSetCollisionCallback(newtonWorld, defaultMaterialID, defaultMaterialID, nullptr, newtonOnAABBOverlap, newtonOnContactFriction);

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

        void onLoadSucceeded(HRESULT resultValue)
        {
        }

        void onFree(void)
        {
            if (newtonWorld)
            {
                NewtonWaitForUpdateToFinish(newtonWorld);
                NewtonSerializeToFile(newtonWorld, CW2A(FileSystem::expandPath(L"$root\\data\\newton.bin")), nullptr, nullptr);
            }

            for (auto &collisionPair : collisionList)
            {
                NewtonDestroyCollision(collisionPair.second);
            }

            collisionList.clear();
            entityMap.clear();
            surfaceList.clear();
            surfaceIndexList.clear();

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

        void onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(entity);

            if (entity->hasComponents<TransformComponent>())
            {
                auto &transformComponent = entity->getComponent<TransformComponent>();
                if (entity->hasComponent<StaticBodyComponent>())
                {
                    auto &staticBodyComponent = entity->getComponent<StaticBodyComponent>();
                    NewtonCollision *newtonCollision = loadCollision(entity, staticBodyComponent.shape);
                    if (newtonCollision != nullptr)
                    {
                        NewtonCollision *clonedCollision = NewtonCollisionCreateInstance(newtonCollision);
                        NewtonCollisionSetMatrix(clonedCollision, transformComponent.getMatrix().data);
                        NewtonSceneCollisionAddSubCollision(newtonStaticScene, clonedCollision);
                        NewtonDestroyCollision(clonedCollision);
                    }
                }
                else if (entity->hasComponents<MassComponent>())
                {
                    auto &massComponent = entity->getComponent<MassComponent>();
                    if (entity->hasComponent<RigidBodyComponent>())
                    {
                        auto &rigidBodyComponent = entity->getComponent<RigidBodyComponent>();
                        NewtonCollision *newtonCollision = loadCollision(entity, rigidBodyComponent.shape);
                        if (newtonCollision != nullptr)
                        {
                            NewtonEntityPtr rigidBody(createRigidBody(newtonWorld, newtonCollision, entity, transformComponent, massComponent));
                            if (rigidBody)
                            {
                                entityMap[entity] = rigidBody;
                                NewtonBodySetTransformCallback(rigidBody->getNewtonBody(), newtonSetTransform);
                            }
                        }
                    }
                    else if (entity->hasComponent<PlayerBodyComponent>())
                    {
                        auto &playerBodyComponent = entity->getComponent<PlayerBodyComponent>();
                        NewtonEntityPtr playerBody(createPlayerBody(engine, newtonWorld, entity, playerBodyComponent, transformComponent, massComponent));
                        if (playerBody)
                        {
                            entityMap[entity] = playerBody;
                            NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                        }
                    }
                }
            }
        }

        void onEntityDestroyed(Entity *entity)
        {
            auto entityIterator = entityMap.find(entity);
            if (entityIterator != entityMap.end())
            {
                entityMap.erase(entityIterator);
            }
        }

        void onUpdate(UINT32 handle, bool isIdle)
        {
            GEK_REQUIRE(population);

            if (!isIdle && newtonWorld)
            {
                NewtonUpdateAsync(newtonWorld, population->getFrameTime());
            }
        }

    private:
        UINT32 getStaticSceneSurface(const Math::Float3 &position, const Math::Float3 &normal)
        {
            dLong surfaceAttribute = 0;
            Math::Float3 collisionNormal;
            NewtonCollisionRayCast(newtonStaticScene, (position - normal).data, (position + normal).data, collisionNormal.data, &surfaceAttribute);
            if (surfaceAttribute > 0)
            {
                return staticSurfaceMap[UINT32(surfaceAttribute)];
            }

            return 0;
        }

        NewtonCollision *createCollision(Entity *entity, const wstring &shape)
        {
            GEK_REQUIRE(population);

            NewtonCollision *newtonCollision = nullptr;
            std::size_t collisionHash = std::hash<wstring>()(shape);
            auto collisionIterator = collisionList.find(collisionHash);
            if (collisionIterator != collisionList.end())
            {
                if ((*collisionIterator).second)
                {
                    newtonCollision = (*collisionIterator).second;
                }
            }
            else
            {
                collisionList[collisionHash] = nullptr;

                std::vector<wstring> parameters(shape.split(L'|'));
                GEK_THROW_ERROR(parameters.size() != 2, BaseException, "Invalid parameters passed for shape: %v", shape);

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
                    newtonCollision = NewtonCreateCapsule(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (parameters[0].compareNoCase(L"*cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateCylinder(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
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
                else if (parameters[0].compareNoCase(L"*chamfer_cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters[1]));
                    newtonCollision = NewtonCreateChamferCylinder(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }

                GEK_THROW_ERROR(newtonCollision == nullptr, BaseException, "Unable to create newton collision shape: %v", shape);
                collisionList[collisionHash] = newtonCollision;
            }

            return newtonCollision;
        }

        NewtonCollision *loadCollision(Entity *entity, const wstring &shape)
        {
            NewtonCollision *newtonCollision = nullptr;
            if (shape.at(0) == L'*')
            {
                newtonCollision = createCollision(entity, shape);
            }
            else
            {
                std::size_t shapeHash = std::hash<wstring>()(shape);
                auto collisionIterator = collisionList.find(shapeHash);
                if (collisionIterator != collisionList.end())
                {
                    if ((*collisionIterator).second)
                    {
                        newtonCollision = (*collisionIterator).second;
                    }
                }
                else
                {
                    collisionList[shapeHash] = nullptr;

                    wstring fileName(FileSystem::expandPath(String::format(L"$root\\data\\models\\%v.bin", shape)));

                    FILE *file = nullptr;
                    _wfopen_s(&file, fileName, L"rb");
                    GEK_THROW_ERROR(file == nullptr, FileSystem::Exception, "Unable to load collision model: %v", fileName);

                    UINT32 gekIdentifier = 0;
                    fread(&gekIdentifier, sizeof(UINT32), 1, file);
                    GEK_THROW_ERROR(gekIdentifier != *(UINT32 *)"GEKX", BaseException, "Invalid model idetifier found: %v", gekIdentifier);

                    UINT16 gekModelType = 0;
                    fread(&gekModelType, sizeof(UINT16), 1, file);

                    UINT16 gekModelVersion = 0;
                    fread(&gekModelVersion, sizeof(UINT16), 1, file);

                    if (gekModelType == 1 && gekModelVersion == 0)
                    {
                        newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, file);
                    }
                    else if (gekModelType == 2 && gekModelVersion == 0)
                    {
                        UINT32 materialCount = 0;
                        fread(&materialCount, sizeof(UINT32), 1, file);
                        for (UINT32 materialIndex = 0; materialIndex < materialCount; materialIndex++)
                        {
                            wstring materialName;
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
                        GEK_THROW_EXCEPTION(BaseException, "Invalid model format/version specified: %v, %v", gekModelType, gekModelVersion);
                    }

                    fclose(file);
                    GEK_THROW_ERROR(newtonCollision == nullptr, BaseException, "Unable to create newton collision shape: %v", shape);
                    collisionList[shapeHash] = newtonCollision;
                }
            }

            return newtonCollision;
        }
    };

    GEK_REGISTER_CONTEXT_USER(NewtonProcessorImplementation)
}; // namespace Gek

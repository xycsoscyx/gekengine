#include "GEK\Context\COM.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Action.h"
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
    extern NewtonEntity *createPlayerBody(IUnknown *actionProvider, NewtonWorld *newtonWorld, Entity *entity, PlayerBodyComponent &playerBodyComponent, TransformComponent &transformComponent, MassComponent &massComponent);
    extern NewtonEntity *createRigidBody(NewtonWorld *newton, const NewtonCollision* const newtonCollision, Entity *entity, TransformComponent &transformComponent, MassComponent &massComponent);

    class NewtonProcessorImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
        , public Processor
        , public NewtonProcessor
    {
    public:
        struct Surface
        {
            bool ghost;
            float staticFriction;
            float kineticFriction;
            float elasticity;
            float softness;

            Surface(void)
                : ghost(false)
                , staticFriction(0.9f)
                , kineticFriction(0.5f)
                , elasticity(0.4f)
                , softness(1.0f)
            {
            }
        };

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
        Population *population;
        UINT32 updateHandle;

        IUnknown *actionProvider;

        NewtonWorld *newtonWorld;
        NewtonCollision *newtonStaticScene;

        Math::Float3 gravity;
        std::vector<Surface> surfaceList;
        std::map<std::size_t, UINT32> surfaceIndexList;
        std::unordered_map<Entity *, CComPtr<NewtonEntity>> entityMap;
        std::unordered_map<std::size_t, NewtonCollision *> collisionList;

    public:
        NewtonProcessorImplementation(void)
            : population(nullptr)
            , updateHandle(0)
            , actionProvider(nullptr)
            , newtonWorld(nullptr)
            , newtonStaticScene(nullptr)
            , gravity(0.0f, -32.174f, 0.0f)
        {
        }

        ~NewtonProcessorImplementation(void)
        {
            onFree();
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
            }

            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(NewtonProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
            INTERFACE_LIST_ENTRY_COM(NewtonProcessor)
        END_INTERFACE_LIST_USER

        const Surface &getSurface(UINT32 surfaceIndex) const
        {
            return surfaceList[surfaceIndex];
        }

        UINT32 getContactSurface(Entity *entity, const NewtonBody *const newtonBody, const NewtonMaterial *newtonMaterial, const Math::Float3 &position, const Math::Float3 &normal)
        {
            if (entity && entity->hasComponent<RigidBodyComponent>())
            {
                auto &rigidBodyComponent = entity->getComponent<RigidBodyComponent>();
                if (!rigidBodyComponent.surface.IsEmpty())
                {
                    return loadSurface(rigidBodyComponent.surface);
                }
            }

            NewtonCollision *newtonCollision = NewtonMaterialGetBodyCollidingShape(newtonMaterial, newtonBody);
            if (newtonCollision)
            {
                dLong surfaceAttribute = 0;
                Math::Float3 collisionNormal;
                NewtonCollisionRayCast(newtonCollision, (position - normal).data, (position + normal).data, collisionNormal.data, &surfaceAttribute);
                if (surfaceAttribute > 0)
                {
                    return UINT32(surfaceAttribute);
                }
            }

            return 0;
        }

        UINT32 loadSurface(LPCWSTR fileName)
        {
            GEK_REQUIRE_RETURN(fileName, 0);

            UINT32 surfaceIndex = 0;
            std::size_t fileNameHash = std::hash<CStringW>()(fileName);
            auto surfaceIterator = surfaceIndexList.find(fileNameHash);
            if (surfaceIterator != surfaceIndexList.end())
            {
                surfaceIndex = (*surfaceIterator).second;
            }
            else
            {
                HRESULT resultValue = E_FAIL;

                surfaceIndexList[fileNameHash] = 0;

                Gek::XmlDocument xmlDocument;
                if (SUCCEEDED(resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName))))
                {
                    Gek::XmlNode xmlMaterialNode = xmlDocument.getRoot();
                    if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"material") == 0)
                    {
                        if (xmlMaterialNode.hasChildElement(L"surface"))
                        {
                            Gek::XmlNode xmlSurfaceNode = xmlMaterialNode.firstChildElement(L"surface");
                            if (xmlSurfaceNode)
                            {
                                Surface surface;
                                surface.ghost = String::to<bool>(xmlSurfaceNode.getAttribute(L"ghost"));
                                if (xmlSurfaceNode.hasAttribute(L"staticfriction"))
                                {
                                    surface.staticFriction = Gek::String::to<float>(xmlSurfaceNode.getAttribute(L"staticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"kineticfriction"))
                                {
                                    surface.kineticFriction = Gek::String::to<float>(xmlSurfaceNode.getAttribute(L"kineticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"elasticity"))
                                {
                                    surface.elasticity = Gek::String::to<float>(xmlSurfaceNode.getAttribute(L"elasticity"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"softness"))
                                {
                                    surface.softness = Gek::String::to<float>(xmlSurfaceNode.getAttribute(L"softness"));
                                }

                                surfaceIndex = surfaceList.size();
                                surfaceIndexList[fileNameHash] = surfaceIndex;
                                surfaceList.push_back(surface);
                            }
                        }
                    }
                }
            }

            return surfaceIndex;
        }

        NewtonCollision *createCollision(Entity *entity, const CStringW &shape)
        {
            GEK_REQUIRE_RETURN(population, nullptr);

            NewtonCollision *newtonCollision = nullptr;
            std::size_t collisionHash = std::hash<CStringW>()(shape);
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
                int position = 0;
                CStringW shapeType(shape.Tokenize(L"|", position));
                CStringW parameters(shape.Tokenize(L"|", position));

                collisionList[collisionHash] = nullptr;
                if (shapeType.CompareNoCase(L"*cube") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters));
                    newtonCollision = NewtonCreateBox(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*sphere") == 0)
                {
                    float size = Evaluator::get<float>(parameters);
                    newtonCollision = NewtonCreateSphere(newtonWorld, size, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*cone") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters));
                    newtonCollision = NewtonCreateCone(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*capsule") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters));
                    newtonCollision = NewtonCreateCapsule(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters));
                    newtonCollision = NewtonCreateCylinder(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*tapered_capsule") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters));
                    newtonCollision = NewtonCreateTaperedCapsule(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*tapered_cylinder") == 0)
                {
                    Math::Float3 size(Evaluator::get<Math::Float3>(parameters));
                    newtonCollision = NewtonCreateTaperedCylinder(newtonWorld, size.x, size.y, size.z, collisionHash, Math::Float4x4::Identity.data);
                }
                else if (shapeType.CompareNoCase(L"*chamfer_cylinder") == 0)
                {
                    Math::Float2 size(Evaluator::get<Math::Float2>(parameters));
                    newtonCollision = NewtonCreateChamferCylinder(newtonWorld, size.x, size.y, collisionHash, Math::Float4x4::Identity.data);
                }

                if (newtonCollision)
                {
                    collisionList[collisionHash] = newtonCollision;
                }
            }

            return newtonCollision;
        }

        NewtonCollision *loadCollision(Entity *entity, const CStringW &shape)
        {
            NewtonCollision *newtonCollision = nullptr;
            if (shape.GetAt(0) == L'*')
            {
                newtonCollision = createCollision(entity, shape);
            }
            else
            {
                std::size_t shapeHash = std::hash<CStringW>()(shape);
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
                    CStringW fileName(FileSystem::expandPath(String::format(L"%%root%%\\data\\models\\%s.bin", shape.GetString())));

                    FILE *file = nullptr;
                    _wfopen_s(&file, fileName, L"rb");
                    if (file)
                    {
                        UINT32 gekIdentifier = 0;
                        fread(&gekIdentifier, sizeof(UINT32), 1, file);

                        UINT16 gekModelType = 0;
                        fread(&gekModelType, sizeof(UINT16), 1, file);

                        UINT16 gekModelVersion = 0;
                        fread(&gekModelVersion, sizeof(UINT16), 1, file);

                        if (gekIdentifier == *(UINT32 *)"GEKX")
                        {
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
                                    CStringW materialName;
                                    wchar_t letter = 0;
                                    do
                                    {
                                        fread(&letter, sizeof(wchar_t), 1, file);
                                        materialName += letter;
                                    } while (letter != 0);

                                    UINT32 surfaceIndex = loadSurface(materialName);
                                }

                                newtonCollision = NewtonCreateCollisionFromSerialization(newtonWorld, deSerializeCollision, file);
                            }
                        }

                        fclose(file);
                    }

                    collisionList[shapeHash] = nullptr;

                    if (newtonCollision)
                    {
                        collisionList[shapeHash] = newtonCollision;
                    }
                }
            }

            return newtonCollision;
        }

        // NewtonProcessor
        STDMETHODIMP_(Math::Float3) getGravity(const Math::Float3 &position)
        {
            const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);
            return Gravity;
        }

        // Processor
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<Population> population(initializerContext);
            if (population)
            {
                this->actionProvider = initializerContext;
                this->population = population;

                updateHandle = population->setUpdatePriority(this, 50);
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
            }

            return resultValue;
        };

        static void newtonPreUpdateTask(NewtonWorld* const world, void* const userData, int threadIndex)
        {
            auto updatePair = *(std::pair<NewtonEntity *, float> *)userData;
            updatePair.first->onPreUpdate(updatePair.second, threadIndex);
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
            NewtonProcessorImplementation *processor = static_cast<NewtonProcessorImplementation *>(NewtonWorldGetUserData(newtonWorld));
            processor->onContactFriction(contactJoint, frameTime, threadHandle);
        }

        void onContactFriction(const NewtonJoint* contactJoint, dFloat frameTime, int threadHandle)
        {
            const NewtonBody* const body0 = NewtonJointGetBody0(contactJoint);
            const NewtonBody* const body1 = NewtonJointGetBody1(contactJoint);
            NewtonEntity *newtonEntity0 = static_cast<NewtonEntity *>(NewtonBodyGetUserData(body0));
            NewtonEntity *newtonEntity1 = static_cast<NewtonEntity *>(NewtonBodyGetUserData(body1));

            NewtonWorldCriticalSectionLock(newtonWorld, threadHandle);
            for (void* newtonContact = NewtonContactJointGetFirstContact(contactJoint); newtonContact; newtonContact = NewtonContactJointGetNextContact(contactJoint, newtonContact))
            {
                NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                Math::Float3 position, normal;
                NewtonMaterialGetContactPositionAndNormal(newtonMaterial, body0, position.data, normal.data);

                UINT32 surfaceIndex0 = getContactSurface((newtonEntity0 ? newtonEntity0->getEntity() : nullptr), body0, newtonMaterial, position, normal);
                UINT32 surfaceIndex1 = getContactSurface((newtonEntity1 ? newtonEntity1->getEntity() : nullptr), body1, newtonMaterial, position, normal);
                const Surface &surface0 = getSurface(surfaceIndex0);
                const Surface &surface1 = getSurface(surfaceIndex1);

                NewtonMaterialSetContactSoftness(newtonMaterial, ((surface0.softness + surface1.softness) * 0.5f));
                NewtonMaterialSetContactElasticity(newtonMaterial, ((surface0.elasticity + surface1.elasticity) * 0.5f));
                NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface0.staticFriction, surface0.kineticFriction, 0);
                NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface1.staticFriction, surface1.kineticFriction, 1);
            }

            NewtonWorldCriticalSectionUnlock(newtonWorld);
        }

        // PopulationObserver
        STDMETHODIMP_(void) onLoadBegin(void)
        {
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

        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            surfaceList.push_back(Surface());
            if (newtonStaticScene)
            {
                NewtonSceneCollisionEndAddRemove(newtonStaticScene);
                if (SUCCEEDED(resultValue))
                {
                    NewtonBody *newtonStaticBody = NewtonCreateDynamicBody(newtonWorld, newtonStaticScene, Math::Float4x4::Identity.data);
                    NewtonBodySetMassProperties(newtonStaticBody, 0.0f, newtonStaticScene);
                }
            }

            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            if (newtonWorld)
            {
                NewtonWaitForUpdateToFinish(newtonWorld);
                NewtonSerializeToFile(newtonWorld, CW2A(FileSystem::expandPath(L"%root%\\data\\newton.bin")), nullptr, nullptr);
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

            if (newtonWorld)
            {
                NewtonDestroyAllBodies(newtonWorld);
                NewtonInvalidateCache(newtonWorld);
                NewtonDestroy(newtonWorld);
                newtonWorld = nullptr;
            }

            GEK_REQUIRE_VOID_RETURN(NewtonGetMemoryUsed() == 0);
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE_VOID_RETURN(entity);

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
                            CComPtr<NewtonEntity> rigidBody = createRigidBody(newtonWorld, newtonCollision, entity, transformComponent, massComponent);
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
                        CComPtr<NewtonEntity> playerBody = createPlayerBody(actionProvider, newtonWorld, entity, playerBodyComponent, transformComponent, massComponent);
                        if (playerBody)
                        {
                            entityMap[entity] = playerBody;
                            NewtonBodySetTransformCallback(playerBody->getNewtonBody(), newtonSetTransform);
                        }
                    }
                }
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            auto entityIterator = entityMap.find(entity);
            if (entityIterator != entityMap.end())
            {
                entityMap.erase(entityIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(bool isIdle)
        {
            GEK_REQUIRE_VOID_RETURN(population);

            if (!isIdle && newtonWorld)
            {
                NewtonUpdateAsync(newtonWorld, population->getFrameTime());
            }
        }
    };

    REGISTER_CLASS(NewtonProcessorImplementation)
}; // namespace Gek

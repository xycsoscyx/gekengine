#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Action.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Scale.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\StaticBody.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include <Newton.h>
#include <dNewton.h>
#include <dNewtonPlayerManager.h>
#include <dNewtonCollision.h>
#include <memory>
#include <map>

#ifdef _DEBUG
#pragma comment(lib, "newton_d.lib")
#pragma comment(lib, "dNewton_d.lib")
#pragma comment(lib, "dContainers_d.lib")
#pragma comment(lib, "dJointLibrary_d.lib")
#else
#pragma comment(lib, "newton.lib")
#pragma comment(lib, "dNewton.lib")
#pragma comment(lib, "dContainers.lib")
#pragma comment(lib, "dJointLibrary.lib")
#endif

namespace Gek
{
    static const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);

    extern CustomPlayerControllerManager *createPlayerManager(NewtonWorld *newton);
    extern NewtonEntity *createPlayerBody(IUnknown *actionProvider, CustomPlayerControllerManager *playerManager, Entity *entity, const PlayerBodyComponent &playerBodyComponent, const TransformComponent &transformComponent, const MassComponent &massComponent);
    extern NewtonEntity *createRigidBody(dNewton *newton, const dNewtonCollision* const newtonCollision, Entity *entity, const TransformComponent &transformComponent, const MassComponent &massComponent);

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

    class NewtonBase : public dNewton
    {
    private:

    public:
        // dNewton
        bool OnBodiesAABBOverlap(const dNewtonBody* const newtonBody0, const dNewtonBody* const newtonBody1, int threadHandle)
        {
            return true;
        }

        bool OnCompoundSubCollisionAABBOverlap(const dNewtonBody* const newtonBody0, const dNewtonCollision* const newtonCollision0, const dNewtonBody* const newtonBody1, const dNewtonCollision* const newtonCollision1, int threadHandle)
        {
            return true;
        }

        void OnContactProcess(dNewtonContactMaterial* const newtonContactMaterial, dFloat frameTime, int threadHandle)
        {
/*
            NewtonEntity *newtonEntity0 = nullptr;//static_cast<NewtonEntity *>(newtonContactMaterial->GetBody0()->GetUserData());
            NewtonEntity *newtonEntity1 = nullptr;//static_cast<NewtonEntity *>(newtonContactMaterial->GetBody1()->GetUserData());
            if (newtonEntity0 && newtonEntity1)
            {
                NewtonWorldCriticalSectionLock(GetNewton(), threadHandle);
                for (void *newtonContact = newtonContactMaterial->GetFirstContact(); newtonContact; newtonContact = newtonContactMaterial->GetNextContact(newtonContact))
                {
                    NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                    Math::Float3 position, normal;
                    NewtonMaterialGetContactPositionAndNormal(newtonMaterial, newtonEntity0->getNewtonBody(), position.data, normal.data);

                    INT32 surfaceIndex0 = getContactSurface(newtonEntity0->getEntity(), newtonEntity0->getNewtonBody(), newtonMaterial, position, normal);
                    INT32 surfaceIndex1 = getContactSurface(newtonEntity1->getEntity(), newtonEntity1->getNewtonBody(), newtonMaterial, position, normal);
                    const Surface &surface0 = getSurface(surfaceIndex0);
                    const Surface &surface1 = getSurface(surfaceIndex1);

                    NewtonMaterialSetContactSoftness(newtonMaterial, ((surface0.softness + surface1.softness) * 0.5f));
                    NewtonMaterialSetContactElasticity(newtonMaterial, ((surface0.elasticity + surface1.elasticity) * 0.5f));
                    NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface0.staticFriction, surface0.kineticFriction, 0);
                    NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface1.staticFriction, surface1.kineticFriction, 1);
                }

                NewtonWorldCriticalSectionUnlock(GetNewton());
            }
*/
        }
    };

    class NewtonProcessorImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
        , public Processor
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
        Population *population;
        UINT32 updateHandle;

        IUnknown *actionProvider;

        NewtonBase *newton;
        dNewtonCollisionScene *newtonStaticScene;
        CustomPlayerControllerManager *newtonPlayerManager;

        Math::Float3 gravity;
        std::vector<Surface> surfaceList;
        std::map<std::size_t, INT32> surfaceIndexList;
        std::unordered_map<Entity *, CComPtr<NewtonEntity>> bodyList;
        std::unordered_map<std::size_t, std::unique_ptr<dNewtonCollision>> collisionList;

    public:
        NewtonProcessorImplementation(void)
            : population(nullptr)
            , updateHandle(0)
            , actionProvider(nullptr)
            , newton(nullptr)
            , newtonStaticScene(nullptr)
            , newtonPlayerManager(nullptr)
            , gravity(0.0f, -9.8331f, 0.0f)
        {
        }

        ~NewtonProcessorImplementation(void)
        {
            onFree();
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(NewtonProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        const Surface &getSurface(INT32 surfaceIndex) const
        {
            if (surfaceIndex >= 0 && surfaceIndex < int(surfaceList.size()))
            {
                return surfaceList[surfaceIndex];
            }
            else
            {
                static const Surface defaultSurface;
                return defaultSurface;
            }
        }

        INT32 getContactSurface(Entity *entity, NewtonBody *newtonBody, NewtonMaterial *newtonMaterial, const Math::Float3 &position, const Math::Float3 &normal)
        {
            REQUIRE_RETURN(population, -1);

            if (entity->hasComponent<RigidBodyComponent>())
            {
                auto &rigidBodyComponent = entity->getComponent<RigidBodyComponent>();
                if (rigidBodyComponent.surface.IsEmpty())
                {
                    NewtonCollision *newtonCollision = NewtonMaterialGetBodyCollidingShape(newtonMaterial, newtonBody);
                    if (newtonCollision)
                    {
                        dLong surfaceAttribute = 0;
                        Math::Float3 collisionNormal;
                        NewtonCollisionRayCast(newtonCollision, (position - normal).data, (position + normal).data, collisionNormal.data, &surfaceAttribute);
                        if (surfaceAttribute > 0)
                        {
                            return INT32(surfaceAttribute);
                        }
                    }
                }
                else
                {
                    return loadSurface(rigidBodyComponent.surface);
                }
            }

            return -1;
        }

        INT32 loadSurface(LPCWSTR fileName)
        {
            REQUIRE_RETURN(fileName, -1);

            INT32 surfaceIndex = -1;
            std::size_t fileNameHash = std::hash<CStringW>()(fileName);
            auto surfaceIterator = surfaceIndexList.find(fileNameHash);
            if (surfaceIterator != surfaceIndexList.end())
            {
                surfaceIndex = (*surfaceIterator).second;
            }
            else
            {
                gekLogScope(fileName);

                surfaceIndexList[fileNameHash] = -1;

                Gek::XmlDocument xmlDocument;
                if (SUCCEEDED(xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName))))
                {
                    Surface surface;
                    Gek::XmlNode xmlMaterialNode = xmlDocument.getRoot();
                    if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"material") == 0)
                    {
                        Gek::XmlNode xmlSurfaceNode = xmlMaterialNode.firstChildElement(L"surface");
                        if (xmlSurfaceNode)
                        {
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

            return surfaceIndex;
        }

        dNewtonCollision *createCollision(Entity *entity, const CStringW &shape)
        {
            REQUIRE_RETURN(population, nullptr);

            Math::Float3 scale(1.0f, 1.0f, 1.0f);
            if (entity->hasComponent<ScaleComponent>())
            {
                scale.set(entity->getComponent<ScaleComponent>());
            }

            dNewtonCollision *newtonCollision = nullptr;
            std::size_t collisionHash = std::hash<CStringW>()(shape);
            collisionHash = std::hash_combine(collisionHash, std::hash<float>()(scale.x), std::hash<float>()(scale.y), std::hash<float>()(scale.z));
            auto collisionIterator = collisionList.find(collisionHash);
            if (collisionIterator != collisionList.end())
            {
                if ((*collisionIterator).second)
                {
                    newtonCollision = (*collisionIterator).second.get();
                }
            }
            else
            {
                gekLogScope();

                int position = 0;
                CStringW shapeType(shape.Tokenize(L"|", position));
                CStringW parameters(shape.Tokenize(L"|", position));

                collisionList[collisionHash].reset();
                if (shapeType.CompareNoCase(L"*cube") == 0)
                {
                    Math::Float3 size(String::to<Math::Float3>(parameters));
                    newtonCollision = new dNewtonCollisionBox(newton, size.x, size.y, size.z, 1);
                }
                else if (shapeType.CompareNoCase(L"*sphere") == 0)
                {
                    float size = String::to<float>(parameters);
                    newtonCollision = new dNewtonCollisionSphere(newton, size, 1);
                }
                else if (shapeType.CompareNoCase(L"*cone") == 0)
                {
                    Math::Float2 size(String::to<Math::Float2>(parameters));
                    newtonCollision = new dNewtonCollisionCone(newton, size.x, size.y, 1);
                }
                else if (shapeType.CompareNoCase(L"*capsule") == 0)
                {
                    Math::Float2 size(String::to<Math::Float2>(parameters));
                    newtonCollision = new dNewtonCollisionCapsule(newton, size.x, size.y, 1);
                }
                else if (shapeType.CompareNoCase(L"*cylinder") == 0)
                {
                    Math::Float2 size(String::to<Math::Float2>(parameters));
                    newtonCollision = new dNewtonCollisionCylinder(newton, size.x, size.y, 1);
                }
                else if (shapeType.CompareNoCase(L"*tapered_capsule") == 0)
                {
                    Math::Float3 size(String::to<Math::Float3>(parameters));
                    newtonCollision = new dNewtonCollisionTaperedCapsule(newton, size.x, size.y, size.z, 1);
                }
                else if (shapeType.CompareNoCase(L"*tapered_cylinder") == 0)
                {
                    Math::Float3 size(String::to<Math::Float3>(parameters));
                    newtonCollision = new dNewtonCollisionTaperedCylinder(newton, size.x, size.y, size.z, 1);
                }
                else if (shapeType.CompareNoCase(L"*chamfer_cylinder") == 0)
                {
                    Math::Float2 size(String::to<Math::Float2>(parameters));
                    newtonCollision = new dNewtonCollisionChamferedCylinder(newton, size.x, size.y, 1);
                }

                if (newtonCollision)
                {
                    newtonCollision->SetScale(scale.x, scale.y, scale.z);
                    collisionList[collisionHash].reset(newtonCollision);
                }
            }

            return newtonCollision;
        }

        void loadModel(LPCWSTR model, std::function<void(std::map<CStringA, Material> &materialList, UINT32 vertexCount, Vertex *vertexList, UINT32 indexCount, UINT16 *indexList)> loadData)
        {
            std::vector<UINT8> fileData;
            HRESULT resultValue = Gek::FileSystem::load(Gek::String::format(L"%%root%%\\data\\models\\%s.gek", model), fileData);
            if (SUCCEEDED(resultValue))
            {
                UINT8 *rawFileData = fileData.data();
                UINT32 gekIdentifier = *((UINT32 *)rawFileData);
                rawFileData += sizeof(UINT32);

                UINT16 gekModelType = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                UINT16 gekModelVersion = *((UINT16 *)rawFileData);
                rawFileData += sizeof(UINT16);

                if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 2)
                {
                    Gek::Shape::AlignedBox alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                    rawFileData += sizeof(Gek::Shape::AlignedBox);

                    UINT32 materialCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    std::map<CStringA, Material> materialList;
                    for (UINT32 surfaceIndex = 0; surfaceIndex < materialCount; ++surfaceIndex)
                    {
                        CStringA materialName = rawFileData;
                        rawFileData += (materialName.GetLength() + 1);

                        Material &material = materialList[materialName];
                        material.firstVertex = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        material.firstIndex = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        material.indexCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);
                    }

                    UINT32 vertexCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    Vertex *vertexList = (Vertex *)rawFileData;
                    rawFileData += (sizeof(Vertex) * vertexCount);

                    UINT32 indexCount = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    UINT16 *indexList = (UINT16 *)rawFileData;

                    loadData(materialList, vertexCount, vertexList, indexCount, indexList);
                }
            }
        }

        dNewtonCollision *loadCollision(Entity *entity, const CStringW &shape)
        {
            dNewtonCollision *newtonCollision = nullptr;
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
                        newtonCollision = (*collisionIterator).second.get();
                    }
                }
                else
                {
                    gekLogScope(shape);

                    collisionList[shapeHash].reset();
                    loadModel(shape, [&](std::map<CStringA, Material> &materialList, UINT32 vertexCount, Vertex *vertexList, UINT32 indexCount, UINT16 *indexList) -> void
                    {
                        if (materialList.empty())
                        {
                            std::vector<Math::Float3> pointCloudList(indexCount);
                            for (UINT32 index = 0; index < indexCount; ++index)
                            {
                                pointCloudList[index] = vertexList[indexList[index]].position;
                            }

                            newtonCollision = new dNewtonCollisionConvexHull(newton, pointCloudList.size(), pointCloudList[0].data, sizeof(Math::Float3), 0.025f, 1);
                        }
                        else
                        {
                            dNewtonCollisionMesh *newtonCollisionMesh = new dNewtonCollisionMesh(newton, 1);
                            if (newtonCollisionMesh != nullptr)
                            {
                                newtonCollisionMesh->BeginFace();
                                for (auto &materialPair : materialList)
                                {
                                    Material &material = materialPair.second;
                                    INT32 surfaceIndex = loadSurface(CA2W(materialPair.first, CP_UTF8));
                                    const Surface &surface = getSurface(surfaceIndex);
                                    if (!surface.ghost)
                                    {
                                        for (UINT32 index = 0; index < material.indexCount; index += 3)
                                        {
                                            Math::Float3 face[3] =
                                            {
                                                vertexList[material.firstVertex + indexList[material.firstIndex + index + 0]].position,
                                                vertexList[material.firstVertex + indexList[material.firstIndex + index + 1]].position,
                                                vertexList[material.firstVertex + indexList[material.firstIndex + index + 2]].position,
                                            };

                                            newtonCollisionMesh->AddFace(3, face[0].data, sizeof(Math::Float3), surfaceIndex);
                                        }
                                    }
                                }

                                newtonCollisionMesh->EndFace();
                                newtonCollision = newtonCollisionMesh;
                            }
                        }
                    });

                    if (newtonCollision)
                    {
                        collisionList[shapeHash].reset(newtonCollision);
                    }
                }
            }

            return newtonCollision;
        }

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            gekLogScope();

            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

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

        // PopulationObserver
        STDMETHODIMP_(void) onLoadBegin(void)
        {
            newton = new NewtonBase();

            newtonStaticScene = new dNewtonCollisionScene(newton, 1);
            if (newtonStaticScene)
            {
                newtonStaticScene->BeginAddRemoveCollision();
            }

            newtonPlayerManager = createPlayerManager(newton->GetNewton());
        }

        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (newtonStaticScene)
            {
                newtonStaticScene->EndAddRemoveCollision();
                if (SUCCEEDED(resultValue))
                {
                    dNewtonBody *newtonStaticBody = new dNewtonBody(newton, 0.0f, newtonStaticScene, nullptr, Math::Float4x4().data, dNewtonBody::m_dynamic, nullptr);
                }
            }

            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            if (newton)
            {
                newton->WaitForUpdateToFinish();
            }

            collisionList.clear();
            bodyList.clear();
            surfaceList.clear();
            surfaceIndexList.clear();

            delete newtonStaticScene;
            newtonStaticScene = nullptr;

            delete newtonPlayerManager;
            newtonPlayerManager = nullptr;

            if (newton)
            {
                newton->DestroyAllBodies();
            }

            delete newton;
            newton = nullptr;

            REQUIRE_VOID_RETURN(NewtonGetMemoryUsed() == 0);
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            REQUIRE_VOID_RETURN(entity);

            if (entity->hasComponents<TransformComponent>())
            {
                auto &transformComponent = entity->getComponent<TransformComponent>();
                if (entity->hasComponent<StaticBodyComponent>())
                {
                    auto &staticBodyComponent = entity->getComponent<StaticBodyComponent>();
                    dNewtonCollision *newtonCollision = loadCollision(entity, staticBodyComponent.shape);
                    if (newtonCollision != nullptr)
                    {
                        dNewtonCollision *clonedCollision = newtonCollision->Clone(newtonCollision->GetShape());
                        Math::Float4x4 matrix(transformComponent.rotation, transformComponent.position);
                        clonedCollision->SetMatrix(matrix.data);
                        newtonStaticScene->AddCollision(clonedCollision);
                    }
                }
                else if (entity->hasComponents<MassComponent>())
                {
                    auto &massComponent = entity->getComponent<MassComponent>();
                    if (entity->hasComponent<RigidBodyComponent>())
                    {
                        auto &rigidBodyComponent = entity->getComponent<RigidBodyComponent>();
                        dNewtonCollision *newtonCollision = loadCollision(entity, rigidBodyComponent.shape);
                        if (newtonCollision != nullptr)
                        {
                            CComPtr<NewtonEntity> dynamicBody = createRigidBody(newton, newtonCollision, entity, transformComponent, massComponent);
                            if (dynamicBody)
                            {
                                bodyList[entity] = dynamicBody;
                            }
                        }
                    }
                    else if (entity->hasComponent<PlayerBodyComponent>())
                    {
                        auto &playerBodyComponent = entity->getComponent<PlayerBodyComponent>();
                        CComPtr<NewtonEntity> playerBody = createPlayerBody(actionProvider, newtonPlayerManager, entity, playerBodyComponent, transformComponent, massComponent);
                        if (playerBody)
                        {
                            bodyList[entity] = playerBody;
                        }
                    }
                }
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            auto bodyIterator = bodyList.find(entity);
            if (bodyIterator != bodyList.end())
            {
                bodyList.erase(bodyIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(float frameTime)
        {
            if (newton && frameTime > 0.0f)
            {
                newton->Update(frameTime);
            }
        }
    };

    REGISTER_CLASS(NewtonProcessorImplementation)
}; // namespace Gek

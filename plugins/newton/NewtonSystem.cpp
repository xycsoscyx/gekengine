#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Engine\ActionInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Size.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\DynamicBody.h"
#include "GEK\Newton\Player.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include <dNewton.h>
#include <dNewtonPlayerManager.h>
#include <dNewtonCollision.h>
#include <dNewtonDynamicBody.h>
#include <concurrent_unordered_map.h>
#include <memory>
#include <map>

#ifdef _DEBUG
    #pragma comment(lib, "newton.lib")
    #pragma comment(lib, "dNewton.lib")
    #pragma comment(lib, "dContainers.lib")
#else
    #pragma comment(lib, "newton.lib")
    #pragma comment(lib, "dNewton.lib")
    #pragma comment(lib, "dContainers.lib")
#endif

namespace Gek
{
    namespace Newton
    {
        class BaseNewtonBody : virtual public BaseUnknown
        {
        private:
            Engine::Population::Interface *population;
            Handle entityHandle;

        public:
            BaseNewtonBody(Engine::Population::Interface *population, Handle entityHandle)
                : population(population)
                , entityHandle(entityHandle)
            {
            }

            virtual ~BaseNewtonBody(void)
            {
            }

            Engine::Population::Interface *getPopulationSystem(void)
            {
                REQUIRE_RETURN(population, nullptr);
                return population;
            }

            Handle getEntityHandle(void) const
            {
                return entityHandle;
            }

            STDMETHOD_(NewtonBody *, getNewtonBody)     (THIS) const PURE;

            // IUnknown
            BEGIN_INTERFACE_LIST(BaseNewtonBody)
            END_INTERFACE_LIST_UNKNOWN
        };

        class DynamicNewtonBody : public BaseNewtonBody
                          , public dNewtonDynamicBody
        {
        public:
            DynamicNewtonBody(Engine::Population::Interface *population, dNewton *newton, const dNewtonCollision* const newtonCollision, Handle entityHandle,
                const Engine::Components::Transform::Data &transformComponent,
                const Mass::Data &massComponent)
                : BaseNewtonBody(population, entityHandle)
                , dNewtonDynamicBody(newton, massComponent, newtonCollision, nullptr, Math::Float4x4(transformComponent.rotation, transformComponent.position).data, NULL)
            {
            }

            ~DynamicNewtonBody(void)
            {
            }

            // BaseNewtonBody
            STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
            {
                return GetNewtonBody();
            }

            // dNewtonBody
            void OnBodyTransform(const dFloat* const newtonMatrix, int threadHandle)
            {
                const Math::Float4x4 &matrix = *reinterpret_cast<const Math::Float4x4 *>(newtonMatrix);
                auto &transformComponent = getPopulationSystem()->getComponent<Engine::Components::Transform::Data>(getEntityHandle(), Engine::Components::Transform::identifier);
                transformComponent.position = matrix.translation;
                transformComponent.rotation = matrix;
            }

            // dNewtonDynamicBody
            void OnForceAndTorque(dFloat frameTime, int threadHandle)
            {
                auto &massComponent = getPopulationSystem()->getComponent<Mass::Data>(getEntityHandle(), Mass::identifier);
                //AddForce((GetNewtonSystem()->GetGravity() * massComponent).xyz);
            }
        };

        class PlayerNewtonBody : public BaseNewtonBody
                     , virtual public dNewtonPlayerManager::dNewtonPlayer
                     , virtual public Engine::Action::Observer
        {
        private:
            float viewAngle;
            concurrency::concurrent_unordered_map<CStringW, float> constantActionList;
            concurrency::concurrent_unordered_map<CStringW, float> singleActionList;

        public:
            PlayerNewtonBody(Engine::Population::Interface *population, dNewtonPlayerManager *newtonPlayerManager, Handle entityHandle,
                const Engine::Components::Transform::Data &transformComponent,
                const Mass::Data &massComponent, 
                const Player::Data &playerComponent)
                : BaseNewtonBody(population, entityHandle)
                , dNewtonPlayerManager::dNewtonPlayer(newtonPlayerManager, nullptr, massComponent, playerComponent.outerRadius, playerComponent.innerRadius,
                    playerComponent.height, playerComponent.stairStep, Math::Float3(0.0f, 1.0f, 0.0f).xyz, Math::Float3(0.0f, 0.0f, 1.0f).xyz, 1)
                , viewAngle(0.0f)
            {
                SetMatrix(Math::Float4x4(transformComponent.rotation, transformComponent.position).data);
            }

            ~PlayerNewtonBody(void)
            {
                //BaseObservable::removeObserver(GetEngineCore(), getClass<Engine::Action::Observer>());
            }

            // BaseNewtonBody
            STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
            {
                return GetNewtonBody();
            }

            // dNewtonBody
            void OnBodyTransform(const dFloat* const newtonMatrix, int threadHandle)
            {
                const Math::Float4x4 &matrix = *reinterpret_cast<const Math::Float4x4 *>(newtonMatrix);
                auto &transformComponent = getPopulationSystem()->getComponent<Engine::Components::Transform::Data>(getEntityHandle(), Engine::Components::Transform::identifier);
                transformComponent.position = matrix.translation;
                transformComponent.rotation = matrix;
            }

            // dNewtonPlayerManager::dNewtonPlayer
            void OnPlayerMove(dFloat frameTime)
            {
                float moveSpeed = 0.0f;
                float strafeSpeed = 0.0f;
                float jumpSpeed = 0.0f;
                static auto checkActions = [&](concurrency::concurrent_unordered_map<CStringW, float> &aActions) -> void
                {
                    viewAngle += (aActions[L"turn"] * 0.01f);

                    moveSpeed += aActions[L"forward"];
                    moveSpeed -= aActions[L"backward"];
                    strafeSpeed += aActions[L"strafe_left"];
                    strafeSpeed -= aActions[L"strafe_right"];
                    jumpSpeed += aActions[L"rise"];
                    jumpSpeed -= aActions[L"fall"];
                };

                checkActions(singleActionList);
                singleActionList.clear();
                checkActions(constantActionList);

                SetPlayerVelocity(moveSpeed, strafeSpeed, jumpSpeed, viewAngle, Math::Float3(0.0f, 0.0f, 0.0f)/*GetNewtonSystem()->GetGravity()*/.xyz, frameTime);
            }

            // Action::ObserverInterface
            STDMETHODIMP_(void) onState(LPCWSTR name, bool state)
            {
                if (state)
                {
                    constantActionList[name] = 10.0f;
                }
                else
                {
                    constantActionList[name] = 0.0f;
                }
            }

            STDMETHODIMP_(void) onValue(LPCWSTR name, float value)
            {
                singleActionList[name] = value;
            }
        };

        class System : public Context::BaseUser
                     , public BaseObservable
                     , public Engine::Population::Observer
                     , public Engine::System::Interface
                     , public dNewton
        {
        public:
            struct Surface
            {
                float staticFriction;
                float kineticFriction;
                float elasticity;
                float softness;

                Surface(void)
                    : staticFriction(0.9f)
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
            Engine::Population::Interface *population;

            dNewtonPlayerManager *newtonPlayerManager;

            Math::Float3 gravity;
            std::vector<Surface> surfaceList;
            std::map<CStringW, INT32> surfaceIndexList;
            concurrency::concurrent_unordered_map<Handle, CComPtr<IUnknown>> bodyList;
            std::unordered_map<CStringW, std::unique_ptr<dNewtonCollision>> collisionList;

        public:
            System(void)
                : population(nullptr)
                , gravity(0.0f, -9.8331f, 0.0f)
            {
            }

            ~System(void)
            {
                BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());

                bodyList.clear();
                collisionList.clear();
                surfaceList.clear();
                surfaceIndexList.clear();
                DestroyAllBodies();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::System::Interface)
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

            INT32 getContactSurface(Handle entityHandle, NewtonBody *newtonBody, NewtonMaterial *newtonMaterial, const Math::Float3 &position, const Math::Float3 &normal)
            {
                REQUIRE_RETURN(population, -1);

                if (population->hasComponent(entityHandle, DynamicBody::identifier))
                {
                    auto &dynamicBodyComponent = population->getComponent<DynamicBody::Data>(entityHandle, DynamicBody::identifier);
                    if (dynamicBodyComponent.surface.IsEmpty())
                    {
                        NewtonCollision *newtonCollision = NewtonMaterialGetBodyCollidingShape(newtonMaterial, newtonBody);
                        if (newtonCollision)
                        {
                            dLong nAttribute = 0;
                            Math::Float3 nCollisionNormal;
                            NewtonCollisionRayCast(newtonCollision, (position - normal).xyz, (position + normal).xyz, nCollisionNormal.xyz, &nAttribute);
                            if (nAttribute > 0)
                            {
                                return INT32(nAttribute);
                            }
                        }
                    }
                    else
                    {
                        return loadSurface(dynamicBodyComponent.surface);
                    }
                }

                return -1;
            }

            INT32 loadSurface(LPCWSTR fileName)
            {
                REQUIRE_RETURN(fileName, -1);

                INT32 surfaceIndex = -1;
                auto surfaceIterator = surfaceIndexList.find(fileName);
                if (surfaceIterator != surfaceIndexList.end())
                {
                    surfaceIndex = (*surfaceIterator).second;
                }
                else
                {
                    gekLogScope(__FUNCTION__);

                    surfaceIndexList[fileName] = -1;

                    Gek::Xml::Document xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName))))
                    {
                        Surface surface;
                        Gek::Xml::Node xmlMaterialNode = xmlDocument.getRoot();
                        if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"surface") == 0)
                        {
                            Gek::Xml::Node xmlSurfaceNode = xmlMaterialNode.firstChildElement(L"surface");
                            if (xmlSurfaceNode)
                            {
                                if (xmlSurfaceNode.hasAttribute(L"staticfriction"))
                                {
                                    surface.staticFriction = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"staticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"kineticfriction"))
                                {
                                    surface.kineticFriction = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"kineticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"elasticity"))
                                {
                                    surface.elasticity = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"elasticity"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"softness"))
                                {
                                    surface.softness = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"softness"));
                                }

                                surfaceIndex = surfaceList.size();
                                surfaceIndexList[fileName] = surfaceIndex;
                                surfaceList.push_back(surface);
                            }
                        }
                    }
                }

                return surfaceIndex;
            }

            dNewtonCollision *createCollision(Handle entityHandle, const DynamicBody::Data &dynamicBodyComponet)
            {
                REQUIRE_RETURN(population, nullptr);

                Math::Float3 size(1.0f, 1.0f, 1.0f);
                if (population->hasComponent(entityHandle, Engine::Components::Size::identifier))
                {
                    size = population->getComponent<Engine::Components::Size::Data>(entityHandle, Engine::Components::Size::identifier);
                }

                CStringW shape(Gek::String::format(L"%s:%f,%f,%f", dynamicBodyComponet.shape.GetString(), size.x, size.y, size.z));

                dNewtonCollision *newtonCollision = nullptr;
                auto collisionIterator = collisionList.find(shape);
                if (collisionIterator != collisionList.end())
                {
                    if ((*collisionIterator).second)
                    {
                        newtonCollision = (*collisionIterator).second.get();
                    }
                }
                else
                {
                    gekLogScope(__FUNCTION__);

                    collisionList[shape].reset();
                    if (dynamicBodyComponet.shape.CompareNoCase(L"*cube") == 0)
                    {
                        newtonCollision = new dNewtonCollisionBox(this, size.x, size.y, size.z, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*sphere") == 0)
                    {
                        newtonCollision = new dNewtonCollisionSphere(this, size.x, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*cone") == 0)
                    {
                        newtonCollision = new dNewtonCollisionCone(this, size.x, size.y, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*capsule") == 0)
                    {
                        newtonCollision = new dNewtonCollisionCapsule(this, size.x, size.y, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*cylinder") == 0)
                    {
                        newtonCollision = new dNewtonCollisionCylinder(this, size.x, size.y, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*tapered_capsule") == 0)
                    {
                        newtonCollision = new dNewtonCollisionTaperedCapsule(this, size.x, size.y, size.z, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*tapered_cylinder") == 0)
                    {
                        newtonCollision = new dNewtonCollisionTaperedCylinder(this, size.x, size.y, size.z, 1);
                    }
                    else if (dynamicBodyComponet.shape.CompareNoCase(L"*chamfer_cylinder") == 0)
                    {
                        newtonCollision = new dNewtonCollisionChamferedCylinder(this, size.x, size.y, 1);
                    }

                    if (newtonCollision)
                    {
                        collisionList[shape].reset(newtonCollision);
                    }
                }

                return newtonCollision;
            }

            dNewtonCollision *loadCollision(Handle entityHandle, const DynamicBody::Data &dynamicBodyComponet)
            {
                dNewtonCollision *newtonCollision = nullptr;
                if (dynamicBodyComponet.shape.GetAt(0) == L'*')
                {
                    newtonCollision = createCollision(entityHandle, dynamicBodyComponet);
                }
                else
                {
                    auto collisionIterator = collisionList.find(dynamicBodyComponet.shape);
                    if (collisionIterator != collisionList.end())
                    {
                        if ((*collisionIterator).second)
                        {
                            newtonCollision = (*collisionIterator).second.get();
                        }
                    }
                    else
                    {
                        gekLogScope(__FUNCTION__);

                        collisionList[dynamicBodyComponet.shape].reset();

                        std::vector<UINT8> fileData;
                        HRESULT resultValue = Gek::FileSystem::load(Gek::String::format(L"%%root%%\\data\\models\\%s.gek", dynamicBodyComponet.shape.GetString()), fileData);
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

                                if (materialList.empty())
                                {
                                    std::vector<Math::Float3> pointCloudList(indexCount);
                                    for (UINT32 index = 0; index < indexCount; ++index)
                                    {
                                        pointCloudList[index] = vertexList[indexList[index]].position;
                                    }

                                    newtonCollision = new dNewtonCollisionConvexHull(this, pointCloudList.size(), pointCloudList[0].xyz, sizeof(Math::Float3), 0.025f, 1);
                                }
                                else
                                {
                                    dNewtonCollisionMesh *newtonCollisionMesh = new dNewtonCollisionMesh(this, 1);
                                    if (newtonCollisionMesh != nullptr)
                                    {
                                        newtonCollisionMesh->BeginFace();
                                        for (auto &materialPair : materialList)
                                        {
                                            Material &material = materialPair.second;
                                            INT32 surfaceIndex = loadSurface(CA2W(materialPair.first, CP_UTF8));
                                            for (UINT32 index = 0; index < material.indexCount; index += 3)
                                            {
                                                Math::Float3 face[3] =
                                                {
                                                    vertexList[material.firstVertex + indexList[material.firstIndex + index + 0]].position,
                                                    vertexList[material.firstVertex + indexList[material.firstIndex + index + 1]].position,
                                                    vertexList[material.firstVertex + indexList[material.firstIndex + index + 2]].position,
                                                };

                                                newtonCollisionMesh->AddFace(3, face[0].xyz, sizeof(Math::Float3), surfaceIndex);
                                            }
                                        }

                                        newtonCollisionMesh->EndFace();
                                        newtonCollision = newtonCollisionMesh;
                                    }
                                }
                            }
                        }

                        if (newtonCollision)
                        {
                            collisionList[dynamicBodyComponet.shape].reset(newtonCollision);
                        }
                    }
                }

                return newtonCollision;
            }

            // System::Interface
            STDMETHODIMP initialize(IUnknown *initializerContext)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Engine::Population::Interface> population(initializerContext);
                if (population)
                {
                    this->population = population;
                    resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    newtonPlayerManager = new dNewtonPlayerManager(this);
                    resultValue = (newtonPlayerManager ? S_OK : E_OUTOFMEMORY);
                }

                return resultValue;
            };

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
                BaseNewtonBody *baseBody0 = dynamic_cast<BaseNewtonBody *>(newtonContactMaterial->GetBody0());
                BaseNewtonBody *baseBody1 = dynamic_cast<BaseNewtonBody *>(newtonContactMaterial->GetBody1());
                if (baseBody0 && baseBody1)
                {
                    NewtonWorldCriticalSectionLock(GetNewton(), threadHandle);
                    for (void *newtonContact = newtonContactMaterial->GetFirstContact(); newtonContact; newtonContact = newtonContactMaterial->GetNextContact(newtonContact))
                    {
                        NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                        Math::Float3 position, normal;
                        NewtonMaterialGetContactPositionAndNormal(newtonMaterial, baseBody0->getNewtonBody(), position.xyz, normal.xyz);

                        INT32 surfaceIndex0 = getContactSurface(baseBody0->getEntityHandle(), baseBody0->getNewtonBody(), newtonMaterial, position, normal);
                        INT32 surfaceIndex1 = getContactSurface(baseBody1->getEntityHandle(), baseBody1->getNewtonBody(), newtonMaterial, position, normal);
                        const Surface &surface0 = getSurface(surfaceIndex0);
                        const Surface &surface1 = getSurface(surfaceIndex1);

                        NewtonMaterialSetContactSoftness(newtonMaterial, ((surface0.softness + surface1.softness) * 0.5f));
                        NewtonMaterialSetContactElasticity(newtonMaterial, ((surface0.elasticity + surface1.elasticity) * 0.5f));
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface0.staticFriction, surface0.kineticFriction, 0);
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, surface1.staticFriction, surface1.kineticFriction, 1);
                    }
                }

                NewtonWorldCriticalSectionUnlock(GetNewton());
            }

            // Population::ObserverInterface
            STDMETHODIMP_(void) onLoadBegin(void)
            {
            }

            STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
            {
                if (FAILED(resultValue))
                {
                    onFree();
                }
            }

            STDMETHODIMP_(void) onFree(void)
            {
                collisionList.clear();
                bodyList.clear();
                surfaceList.clear();
                surfaceIndexList.clear();
                DestroyAllBodies();
            }

            STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
            {
                REQUIRE_VOID_RETURN(population);

                if (population->hasComponent(entityHandle, Engine::Components::Transform::identifier) &&
                    population->hasComponent(entityHandle, Mass::identifier))
                {
                    auto &massComponent = population->getComponent<Mass::Data>(entityHandle, Mass::identifier);
                    auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entityHandle, Engine::Components::Transform::identifier);
                    if (population->hasComponent(entityHandle, DynamicBody::identifier))
                    {
                        auto &dynamicBodyComponet = population->getComponent<DynamicBody::Data>(entityHandle, DynamicBody::identifier);
                        dNewtonCollision *newtonCollision = loadCollision(entityHandle, dynamicBodyComponet);
                        if (newtonCollision != nullptr)
                        {
                            Math::Float4x4 matrix(transformComponent.rotation, transformComponent.position);
                            CComPtr<IUnknown> dynamicBody = new DynamicNewtonBody(population, this, newtonCollision, entityHandle, transformComponent, massComponent);
                            if (dynamicBody)
                            {
                                bodyList[entityHandle] = dynamicBody;
                            }
                        }
                    }
                    else if (population->hasComponent(entityHandle, Player::identifier))
                    {
                        auto &playerComponent = population->getComponent<Player::Data>(entityHandle, Player::identifier);
                        CComPtr<PlayerNewtonBody> player = new PlayerNewtonBody(population, newtonPlayerManager, entityHandle, transformComponent, massComponent, playerComponent);
                        if (player)
                        {
                            //BaseObservable::addObserver(m_pEngine, player->getClass<Engine::Action::Observer>());
                            player.QueryInterface(&bodyList[entityHandle]);
                        }
                    }
                }
            }

            STDMETHODIMP_(void) onEntityDestroyed(Handle entityHandle)
            {
                auto bodyIterator = bodyList.find(entityHandle);
                if (bodyIterator != bodyList.end())
                {
                    bodyList.unsafe_erase(bodyIterator);
                }
            }

            STDMETHODIMP_(void) onUpdate(float frameTime)
            {
                if (frameTime > 0.0f)
                {
                    UpdateOffLine(frameTime);
                }
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Newton
}; // namespace Gek

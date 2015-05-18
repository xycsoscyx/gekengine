#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\Observable.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\ComponentSystemInterface.h"
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
        class BaseBody : public IUnknown
        {
        private:
            ULONG referenceCount;
            Population::Interface *populationSystem;
            Handle entityHandle;

        public:
            BaseBody(Population::Interface *populationSystem, Handle entityHandle)
                : referenceCount(0)
                , populationSystem(populationSystem)
                , entityHandle(entityHandle)
            {
            }

            virtual ~BaseBody(void)
            {
            }

            Population::Interface *getPopulationSystem(void)
            {
                return populationSystem;
            }

            Handle getEntityHandle(void) const
            {
                return entityHandle;
            }

            STDMETHOD_(NewtonBody *, getNewtonBody)     (THIS) const PURE;

            // IUnknown
            STDMETHODIMP_(ULONG) AddRef(void)
            {
                return InterlockedIncrement(&referenceCount);
            }

            STDMETHODIMP_(ULONG) Release(void)
            {
                LONG currentReferenceCount = InterlockedDecrement(&referenceCount);
                if (currentReferenceCount == 0)
                {
                    delete this;
                }

                return currentReferenceCount;
            }

            STDMETHODIMP QueryInterface(REFIID interfaceType, LPVOID FAR *returnObject)
            {
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                HRESULT resultValue = E_INVALIDARG;
                if (IsEqualIID(IID_IUnknown, interfaceType))
                {
                    AddRef();
                    (*returnObject) = dynamic_cast<IUnknown *>(this);
                    _ASSERTE(*returnObject);
                    resultValue = S_OK;
                }

                return resultValue;
            }
        };

        class DynamicBody : public BaseBody
                          , public dNewtonDynamicBody
        {
        public:
            DynamicBody(Population::Interface *populationSystem, dNewton *newton, const dNewtonCollision* const newtonCollision, Handle entityHandle,
                const Components::Transform::Data &transformComponent,
                const Components::Mass::Data &massComponent)
                : BaseBody(populationSystem, entityHandle)
                , dNewtonDynamicBody(newton, massComponent, newtonCollision, nullptr, Math::Float4x4(transformComponent.rotation, transformComponent.position).data, NULL)
            {
            }

            ~DynamicBody(void)
            {
            }

            // BaseBody
            STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
            {
                return GetNewtonBody();
            }

            // dNewtonBody
            void OnBodyTransform(const dFloat* const newtonMatrix, int threadHandle)
            {
                const Math::Float4x4 &matrix = *reinterpret_cast<const Math::Float4x4 *>(newtonMatrix);
                auto &transformComponent = getPopulationSystem()->getComponent<Components::Transform::Data>(getEntityHandle(), Components::Transform::identifier);
                transformComponent.position = matrix.translation;
                transformComponent.rotation = matrix;
            }

            // dNewtonDynamicBody
            void OnForceAndTorque(dFloat frameTime, int threadHandle)
            {
                auto &massComponent = getPopulationSystem()->getComponent<Components::Mass::Data>(getEntityHandle(), Components::Mass::identifier);
                //AddForce((GetNewtonSystem()->GetGravity() * massComponent).xyz);
            }
        };

        class Player : public BaseBody
                     , public dNewtonPlayerManager::dNewtonPlayer
                     //, public Action::ObserverInterface
        {
        private:
            float viewAngle;
            concurrency::concurrent_unordered_map<CStringW, float> constantActionList;
            concurrency::concurrent_unordered_map<CStringW, float> singleActionList;

        public:
            Player(Population::Interface *populationSystem, dNewtonPlayerManager *newtonPlayerManager, Handle entityHandle, 
                const Components::Transform::Data &transformComponent,
                const Components::Mass::Data &massComponent, 
                const Components::Player::Data &playerComponent)
                : BaseBody(populationSystem, entityHandle)
                , dNewtonPlayerManager::dNewtonPlayer(newtonPlayerManager, nullptr, massComponent, playerComponent.outerRadius, playerComponent.innerRadius,
                    playerComponent.height, playerComponent.stairStep, Math::Float3(0.0f, 1.0f, 0.0f).xyz, Math::Float3(0.0f, 0.0f, 1.0f).xyz, 1)
                , viewAngle(0.0f)
            {
                SetMatrix(Math::Float4x4(transformComponent.rotation, transformComponent.position).data);
            }

            ~Player(void)
            {
                //Observable::removeObserver(GetEngineCore(), dynamic_cast<Action::ObserverInterface *>(this));
            }

            // BaseBody
            STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
            {
                return GetNewtonBody();
            }

            // dNewtonBody
            void OnBodyTransform(const dFloat* const newtonMatrix, int threadHandle)
            {
                const Math::Float4x4 &matrix = *reinterpret_cast<const Math::Float4x4 *>(newtonMatrix);
                auto &transformComponent = getPopulationSystem()->getComponent<Components::Transform::Data>(getEntityHandle(), Components::Transform::identifier);
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

                auto &playerComponent = getPopulationSystem()->getComponent<Components::Player::Data>(getEntityHandle(), Components::Player::identifier);
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
                     , public Observable
                     , public ComponentSystemInterface
                     , public Population::ObserverInterface
                     , public dNewton
        {
        public:
            struct Material
            {
                float staticFriction;
                float kineticFriction;
                float elasticity;
                float softness;

                Material(void)
                    : staticFriction(0.9f)
                    , kineticFriction(0.5f)
                    , elasticity(0.4f)
                    , softness(0.1f)
                {
                }
            };

        private:
            Population::Interface *populationSystem;

            dNewtonPlayerManager *playerManager;

            Math::Float3 gravity;
            std::vector<Material> materialList;
            std::map<CStringW, INT32> materialIndexList;
            concurrency::concurrent_unordered_map<Handle, CComPtr<IUnknown>> bodyList;
            std::unordered_map<CStringW, std::unique_ptr<dNewtonCollision>> collisionList;

        public:
            System(void)
                : populationSystem(nullptr)
                , gravity(0.0f, -9.8331f, 0.0f)
            {
            }

            ~System(void)
            {
                Observable::removeObserver(populationSystem, getClass<Population::ObserverInterface>());

                bodyList.clear();
                collisionList.clear();
                materialList.clear();
                materialIndexList.clear();
                DestroyAllBodies();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Population::ObserverInterface)
            END_INTERFACE_LIST_UNKNOWN

            const Material &getMaterial(INT32 materialIndex) const
            {
                if (materialIndex >= 0 && materialIndex < int(materialList.size()))
                {
                    return materialList[materialIndex];
                }
                else
                {
                    static const Material defaultMaterial;
                    return defaultMaterial;
                }
            }

            INT32 getContactMaterial(Handle entityHandle, NewtonBody *newtonBody, NewtonMaterial *newtonMaterial, const Math::Float3 &position, const Math::Float3 &normal)
            {
                if (populationSystem->hasComponent(entityHandle, Components::DynamicBody::identifier))
                {
                    auto &dynamicBodyComponent = populationSystem->getComponent<Components::DynamicBody::Data>(entityHandle, Components::DynamicBody::identifier);
                    if (dynamicBodyComponent.material.IsEmpty())
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
                        return loadMaterial(dynamicBodyComponent.material);
                    }
                }

                return -1;
            }

            INT32 loadMaterial(LPCWSTR fileName)
            {
                REQUIRE_RETURN(fileName, -1);

                INT32 materialIndex = -1;
                auto materialIterator = materialIndexList.find(fileName);
                if (materialIterator != materialIndexList.end())
                {
                    materialIndex = (*materialIterator).second;
                }
                else
                {
                    materialIndexList[fileName] = -1;

                    Gek::Xml::Document xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName))))
                    {
                        Material material;
                        Gek::Xml::Node &xmlMaterialNode = xmlDocument.getRoot();
                        if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"material") == 0)
                        {
                            Gek::Xml::Node &xmlSurfaceNode = xmlMaterialNode.firstChildElement(L"surface");
                            if (xmlSurfaceNode)
                            {
                                if (xmlSurfaceNode.hasAttribute(L"staticfriction"))
                                {
                                    material.staticFriction = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"staticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"kineticfriction"))
                                {
                                    material.kineticFriction = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"kineticfriction"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"elasticity"))
                                {
                                    material.elasticity = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"elasticity"));
                                }

                                if (xmlSurfaceNode.hasAttribute(L"softness"))
                                {
                                    material.softness = Gek::String::getFloat(xmlSurfaceNode.getAttribute(L"softness"));
                                }

                                materialIndex = materialList.size();
                                materialIndexList[fileName] = materialIndex;
                                materialList.push_back(material);
                            }
                        }
                    }
                }

                return materialIndex;
            }

            dNewtonCollision *createCollision(Handle entityHandle, const Components::DynamicBody::Data &dynamicBodyComponet)
            {
                Math::Float3 size(1.0f, 1.0f, 1.0f);
                if (populationSystem->hasComponent(entityHandle, Components::Size::identifier))
                {
                    size = populationSystem->getComponent<Components::Size::Data>(entityHandle, Components::Size::identifier);
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

            dNewtonCollision *loadCollision(Handle entityHandle, const Components::DynamicBody::Data &dynamicBodyComponet)
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
                        collisionList[dynamicBodyComponet.shape].reset();

                        std::vector<UINT8> fileData;
                        HRESULT resultValue = Gek::FileSystem::load(Gek::String::format(L"%%root%%\\data\\models\\%s.gek", dynamicBodyComponet.shape.GetString()), fileData);
                        if (SUCCEEDED(resultValue))
                        {
                            UINT8 *rawFileData = &fileData[0];
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

                                struct RenderMaterial
                                {
                                    UINT32 firstVertex;
                                    UINT32 firstIndex;
                                    UINT32 indexCount;
                                };

                                std::map<CStringA, RenderMaterial> renderMaterialList;
                                for (UINT32 materialIndex = 0; materialIndex < materialCount; ++materialIndex)
                                {
                                    CStringA renderMaterialName = rawFileData;
                                    rawFileData += (renderMaterialName.GetLength() + 1);

                                    RenderMaterial &renderMaterial = renderMaterialList[renderMaterialName];
                                    renderMaterial.firstVertex = *((UINT32 *)rawFileData);
                                    rawFileData += sizeof(UINT32);

                                    renderMaterial.firstIndex = *((UINT32 *)rawFileData);
                                    rawFileData += sizeof(UINT32);

                                    renderMaterial.indexCount = *((UINT32 *)rawFileData);
                                    rawFileData += sizeof(UINT32);
                                }

                                UINT32 vertexCount = *((UINT32 *)rawFileData);
                                rawFileData += sizeof(UINT32);

                                Math::Float3 *vertexList = (Math::Float3 *)rawFileData;
                                rawFileData += (sizeof(Math::Float3) * vertexCount);
                                rawFileData += (sizeof(Math::Float2) * vertexCount);
                                rawFileData += (sizeof(Math::Float3) * vertexCount);

                                UINT32 indexCount = *((UINT32 *)rawFileData);
                                rawFileData += sizeof(UINT32);

                                UINT16 *indexList = (UINT16 *)rawFileData;

                                if (renderMaterialList.empty())
                                {
                                    std::vector<Math::Float3> pointCloudList(indexCount);
                                    for (UINT32 index = 0; index < indexCount; ++index)
                                    {
                                        pointCloudList[index] = vertexList[indexList[index]];
                                    }

                                    newtonCollision = new dNewtonCollisionConvexHull(this, pointCloudList.size(), pointCloudList[0].xyz, sizeof(Math::Float3), 0.025f, 1);
                                }
                                else
                                {
                                    dNewtonCollisionMesh *newtonCollisionMesh = new dNewtonCollisionMesh(this, 1);
                                    if (newtonCollisionMesh != nullptr)
                                    {
                                        newtonCollisionMesh->BeginFace();
                                        for (auto &renderMaterial : renderMaterialList)
                                        {
                                            RenderMaterial &currentMaterial = renderMaterial.second;
                                            INT32 materialIndex = loadMaterial(CA2W(renderMaterial.first, CP_UTF8));
                                            for (UINT32 index = 0; index < currentMaterial.indexCount; index += 3)
                                            {
                                                Math::Float3 face[3] =
                                                {
                                                    vertexList[currentMaterial.firstVertex + indexList[currentMaterial.firstIndex + index + 0]],
                                                    vertexList[currentMaterial.firstVertex + indexList[currentMaterial.firstIndex + index + 1]],
                                                    vertexList[currentMaterial.firstVertex + indexList[currentMaterial.firstIndex + index + 2]],
                                                };

                                                newtonCollisionMesh->AddFace(3, face[0].xyz, sizeof(Math::Float3), materialIndex);
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

            // ComponentSystemInterface
            STDMETHODIMP initialize(Population::Interface *populationSystem)
            {
                REQUIRE_RETURN(populationSystem, E_INVALIDARG);

                this->populationSystem = populationSystem;
                HRESULT resultValue = Observable::addObserver(populationSystem, getClass<Population::ObserverInterface>());
                if (SUCCEEDED(resultValue))
                {
                    playerManager = new dNewtonPlayerManager(this);
                    resultValue = (playerManager ? S_OK : E_OUTOFMEMORY);
                }

                return resultValue;
            };

            // dNewton
            bool OnBodiesAABBOverlap(const dNewtonBody* const newtonBody0, const dNewtonBody* const newtonBody1, int threadHandle) const
            {
                return true;
            }

            bool OnCompoundSubCollisionAABBOverlap(const dNewtonBody* const newtonBody0, const dNewtonCollision* const newtonCollision0, const dNewtonBody* const newtonBody1, const dNewtonCollision* const newtonCollision1, int threadHandle) const
            {
                return true;
            }

            void OnContactProcess(dNewtonContactMaterial* const newtonContactMaterial, dFloat frameTime, int threadHandle)
            {
                BaseBody *baseBody0 = dynamic_cast<BaseBody *>(newtonContactMaterial->GetBody0());
                BaseBody *baseBody1 = dynamic_cast<BaseBody *>(newtonContactMaterial->GetBody1());
                if (baseBody0 && baseBody1)
                {
                    NewtonWorldCriticalSectionLock(GetNewton(), threadHandle);
                    for (void *newtonContact = newtonContactMaterial->GetFirstContact(); newtonContact; newtonContact = newtonContactMaterial->GetNextContact(newtonContact))
                    {
                        NewtonMaterial *newtonMaterial = NewtonContactGetMaterial(newtonContact);

                        Math::Float3 position, normal;
                        NewtonMaterialGetContactPositionAndNormal(newtonMaterial, baseBody0->getNewtonBody(), position.xyz, normal.xyz);

                        INT32 materialIndex0 = getContactMaterial(baseBody0->getEntityHandle(), baseBody0->getNewtonBody(), newtonMaterial, position, normal);
                        INT32 materialIndex1 = getContactMaterial(baseBody1->getEntityHandle(), baseBody1->getNewtonBody(), newtonMaterial, position, normal);
                        const Material &material0 = getMaterial(materialIndex0);
                        const Material &material1 = getMaterial(materialIndex1);

                        NewtonMaterialSetContactSoftness(newtonMaterial, ((material0.softness + material1.softness) * 0.5f));
                        NewtonMaterialSetContactElasticity(newtonMaterial, ((material0.elasticity + material1.elasticity) * 0.5f));
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, material0.staticFriction, material0.kineticFriction, 0);
                        NewtonMaterialSetContactFrictionCoef(newtonMaterial, material1.staticFriction, material1.kineticFriction, 1);
                    }
                }

                NewtonWorldCriticalSectionUnlock(GetNewton());
            }

            // Population::ObserverInterface
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
                materialList.clear();
                materialIndexList.clear();
                DestroyAllBodies();
            }

            STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
            {
                if (populationSystem->hasComponent(entityHandle, Components::Transform::identifier) &&
                    populationSystem->hasComponent(entityHandle, Components::Mass::identifier))
                {
                    auto &massComponent = populationSystem->getComponent<Components::Mass::Data>(entityHandle, Components::Mass::identifier);
                    auto &transformComponent = populationSystem->getComponent<Components::Transform::Data>(entityHandle, Components::Transform::identifier);
                    if (populationSystem->hasComponent(entityHandle, Components::DynamicBody::identifier))
                    {
                        auto &dynamicBodyComponet = populationSystem->getComponent<Components::DynamicBody::Data>(entityHandle, Components::DynamicBody::identifier);
                        dNewtonCollision *newtonCollision = loadCollision(entityHandle, dynamicBodyComponet);
                        if (newtonCollision != nullptr)
                        {
                            Math::Float4x4 matrix(transformComponent.rotation, transformComponent.position);
                            CComPtr<IUnknown> dynamicBody = new DynamicBody(populationSystem, this, newtonCollision, entityHandle, transformComponent, massComponent);
                            if (dynamicBody)
                            {
                                bodyList[entityHandle] = dynamicBody;
                            }
                        }
                    }
                    else if (populationSystem->hasComponent(entityHandle, Components::Player::identifier))
                    {
                        auto &playerComponent = populationSystem->getComponent<Components::Player::Data>(entityHandle, Components::Player::identifier);
                        CComPtr<IUnknown> player = new Player(populationSystem, playerManager, entityHandle, transformComponent, massComponent, playerComponent);
                        if (player)
                        {
                            //Observable::addObserver(m_pEngine, dynamic_cast<Action::ObserverInterface *>((IUnknown *)player));
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
                UpdateOffLine(frameTime);
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Newton
}; // namespace Gek

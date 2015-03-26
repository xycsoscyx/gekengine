#include "CGEKComponentNewton.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

#pragma comment(lib, "newton.lib")

REGISTER_COMPONENT(newton)
    REGISTER_COMPONENT_DEFAULT_VALUE(shape, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(params, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(material, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(mass, 0.0f)
    REGISTER_COMPONENT_SERIALIZE(newton)
        REGISTER_COMPONENT_SERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(newton)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass, StrToFloat)
END_REGISTER_COMPONENT(newton)

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKNewton)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemNewton)

CGEKComponentSystemNewton::CGEKComponentSystemNewton(void)
    : m_pEngine(nullptr)
    , m_pWorld(nullptr)
    , m_nGravity(0.0f, -9.8331f, 0.0f)
{
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
}

CGEKComponentSystemNewton::MATERIAL *CGEKComponentSystemNewton::LoadMaterial(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, nullptr);

    MATERIAL *pMaterial = nullptr;
    auto pIterator = m_aMaterials.find(pName);
    if (pIterator != m_aMaterials.end())
    {
        pMaterial = &(*pIterator).second;
    }
    else
    {
        CLibXMLDoc kDocument;
        if (SUCCEEDED(kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName))))
        {
            CLibXMLNode kMaterialNode = kDocument.GetRoot();
            if (kMaterialNode)
            {
                CLibXMLNode kSurfaceNode = kMaterialNode.FirstChildElement(L"surface");
                if (kSurfaceNode)
                {
                    MATERIAL &kMaterial = m_aMaterials[pName];
                    if (kSurfaceNode.HasAttribute(L"staticfriction"))
                    {
                        kMaterial.m_nStaticFriction = StrToFloat(kSurfaceNode.GetAttribute(L"staticfriction"));
                    }

                    if (kSurfaceNode.HasAttribute(L"kineticfriction"))
                    {
                        kMaterial.m_nKineticFriction = StrToFloat(kSurfaceNode.GetAttribute(L"kineticfriction"));
                    }

                    if (kSurfaceNode.HasAttribute(L"elasticity"))
                    {
                        kMaterial.m_nElasticity = StrToFloat(kSurfaceNode.GetAttribute(L"elasticity"));
                    }

                    if (kSurfaceNode.HasAttribute(L"softness"))
                    {
                        kMaterial.m_nSoftness = StrToFloat(kSurfaceNode.GetAttribute(L"softness"));
                    }

                    pMaterial = &kMaterial;
                }
            }
        }
    }

    if (pMaterial)
    {
        return pMaterial;
    }
    else
    {
        return &m_kDefaultMaterial;
    }
}

NewtonCollision *CGEKComponentSystemNewton::LoadCollision(LPCWSTR pShape, LPCWSTR pParams)
{
    CStringW strIdentity;
    strIdentity.Format(L"%s|%s", pShape, pParams);
    auto pIterator = m_aCollisions.find(strIdentity);
    if (pIterator != m_aCollisions.end())
    {
        return (*pIterator).second;
    }

    size_t nHash = std::hash<LPCWSTR>()(strIdentity.GetString());

    float4x4 nIdentityMatrix;
    NewtonCollision *pCollision = nullptr;
    if (_wcsicmp(pShape, L"convex_hull") == 0 ||
        _wcsicmp(pShape, L"tree") == 0)
    {
        std::vector<UINT8> aBuffer;
        HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.gek", pParams), aBuffer);
        if (SUCCEEDED(hRetVal))
        {
            UINT8 *pBuffer = &aBuffer[0];
            UINT32 nGEKX = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            UINT16 nType = *((UINT16 *)pBuffer);
            pBuffer += sizeof(UINT16);

            UINT16 nVersion = *((UINT16 *)pBuffer);
            pBuffer += sizeof(UINT16);

            if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
            {
                aabb nAABB = *(aabb *)pBuffer;
                pBuffer += sizeof(aabb);

                UINT32 nNumMaterials = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                struct RENDERMATERIAL
                {
                    UINT32 m_nFirstVertex;
                    UINT32 m_nFirstIndex;
                    UINT32 m_nNumIndices;
                };

                std::map<CStringW, RENDERMATERIAL> aMaterials;
                for (UINT32 nMaterial = 0; nMaterial < nNumMaterials; ++nMaterial)
                {
                    CStringA strMaterialUTF8 = pBuffer;
                    pBuffer += (strMaterialUTF8.GetLength() + 1);
                    CStringW strMaterial(CA2W(strMaterialUTF8, CP_UTF8));

                    RENDERMATERIAL &kMaterial = aMaterials[strMaterial];
                    kMaterial.m_nFirstVertex = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    kMaterial.m_nFirstIndex = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    kMaterial.m_nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);
                }

                UINT32 nNumVertices = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                float3 *pVertices = (float3 *)pBuffer;
                pBuffer += (sizeof(float3) * nNumVertices);
                pBuffer += (sizeof(float2) * nNumVertices);
                pBuffer += (sizeof(float3) * nNumVertices);

                UINT32 nNumIndices = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                UINT16 *pIndices = (UINT16 *)pBuffer;

                if (_wcsicmp(pShape, L"convex_hull") == 0 && aMaterials.size() > 0)
                {
                    std::vector<float3> aCloud(nNumIndices);
                    for (UINT32 nIndex = 0; nIndex < nNumIndices; ++nIndex)
                    {
                        aCloud[nIndex] = pVertices[pIndices[nIndex]];
                    }

                    pCollision = NewtonCreateConvexHull(m_pWorld, aCloud.size(), aCloud[0].xyz, sizeof(float3), 0.025f, nHash, nIdentityMatrix.data);
                }
                else if (_wcsicmp(pShape, L"tree") == 0)
                {
                    pCollision = NewtonCreateTreeCollision(m_pWorld, nHash);
                    if (pCollision != nullptr)
                    {
                        NewtonTreeCollisionBeginBuild(pCollision);
                        for (auto &kPair : aMaterials)
                        {
                            RENDERMATERIAL &kMaterial = kPair.second;
                            MATERIAL *pMaterial = LoadMaterial(kPair.first);
                            for (UINT32 nIndex = 0; nIndex < kMaterial.m_nNumIndices; nIndex += 3)
                            {
                                float3 aFace[3] =
                                {
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 0]],
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 1]],
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 2]],
                                };

                                NewtonTreeCollisionAddFace(pCollision, 3, aFace[0].xyz, sizeof(float3), int(pMaterial));
                            }
                        }

                        NewtonTreeCollisionEndBuild(pCollision, false);
                    }
                }
            }
        }
    }
    else
    {
        if (_wcsicmp(pShape, L"cube") == 0)
        {
            float3 nSize = StrToFloat3(pParams);
            pCollision = NewtonCreateBox(m_pWorld, nSize.x, nSize.y, nSize.z, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"sphere") == 0)
        {
            float nSize = StrToFloat(pParams);
            pCollision = NewtonCreateSphere(m_pWorld, nSize, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"cone") == 0)
        {
            float2 nSize = StrToFloat2(pParams);
            pCollision = NewtonCreateCone(m_pWorld, nSize.x, nSize.y, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"capsule") == 0)
        {
            float2 nSize = StrToFloat2(pParams);
            pCollision = NewtonCreateCapsule(m_pWorld, nSize.x, nSize.y, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"cylinder") == 0)
        {
            float2 nSize = StrToFloat2(pParams);
            pCollision = NewtonCreateCylinder(m_pWorld, nSize.x, nSize.y, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"tapered_capsule") == 0)
        {
            float3 nSize = StrToFloat3(pParams);
            pCollision = NewtonCreateTaperedCapsule(m_pWorld, nSize.x, nSize.y, nSize.z, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"tapered_cylinder") == 0)
        {
            float3 nSize = StrToFloat3(pParams);
            pCollision = NewtonCreateTaperedCylinder(m_pWorld, nSize.x, nSize.y, nSize.z, nHash, nIdentityMatrix.data);
        }
        else if (_wcsicmp(pShape, L"chamfer_cylinder") == 0)
        {
            float2 nSize = StrToFloat2(pParams);
            pCollision = NewtonCreateChamferCylinder(m_pWorld, nSize.x, nSize.y, nHash, nIdentityMatrix.data);
        }
    }

    if (pCollision)
    {
        m_aCollisions[strIdentity] = pCollision;
    }

    return pCollision;
}

void CGEKComponentSystemNewton::OnSetForceAndTorque(const NewtonBody *pBody, const GEKENTITYID &nEntityID)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);
    REQUIRE_VOID_RETURN(pBody);

    auto &kNewton = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID, GET_COMPONENT_ID(newton));
    NewtonBodyAddForce(pBody, (m_nGravity * kNewton.mass).xyz);
}

void CGEKComponentSystemNewton::OnEntityTransformed(const NewtonBody *pBody, const GEKENTITYID &nEntityID, const float4x4 &nMatrix)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);
    REQUIRE_VOID_RETURN(pBody);

    auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
    kTransform.position = nMatrix.t;
    kTransform.rotation = nMatrix;
}

void CGEKComponentSystemNewton::OnCollisionContact(const NewtonMaterial *pMaterial, NewtonBody *pBody0, const GEKENTITYID &nEntityID0, NewtonBody *pBody1, const GEKENTITYID &nEntityID1)
{
    auto &kNewton0 = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID0, GET_COMPONENT_ID(newton));
    MATERIAL *pMaterial0 = LoadMaterial(kNewton0.material);

    auto &kNewton1 = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID1, GET_COMPONENT_ID(newton));
    MATERIAL *pMaterial1 = LoadMaterial(kNewton1.material);

    UINT32 nFaceAttribute = NewtonMaterialGetContactFaceAttribute(pMaterial);
    if (nFaceAttribute > 0)
    {
        if (kNewton0.material.CompareNoCase(L"tree") == 0)
        {
            pMaterial0 = (MATERIAL *)nFaceAttribute;
        }
        else if (kNewton1.material.CompareNoCase(L"tree") == 0)
        {
            pMaterial1 = (MATERIAL *)nFaceAttribute;
        }
    }

    NewtonMaterialSetContactSoftness(pMaterial, ((pMaterial0->m_nSoftness + pMaterial1->m_nSoftness) * 0.5f));
    NewtonMaterialSetContactElasticity(pMaterial, ((pMaterial0->m_nElasticity + pMaterial1->m_nElasticity) * 0.5f));
    NewtonMaterialSetContactFrictionCoef(pMaterial, pMaterial0->m_nStaticFriction, pMaterial0->m_nKineticFriction, 0);
    NewtonMaterialSetContactFrictionCoef(pMaterial, pMaterial1->m_nStaticFriction, pMaterial1->m_nKineticFriction, 1);

    float3 nPosition, nNormal;
    NewtonMaterialGetContactPositionAndNormal(pMaterial, pBody0, nPosition.xyz, nNormal.xyz);
    CGEKObservable::SendEvent(TGEKEvent<IGEKNewtonObserver>(std::bind(&IGEKNewtonObserver::OnCollision, std::placeholders::_1, nEntityID0, nEntityID1, nPosition, nNormal)));
}

STDMETHODIMP CGEKComponentSystemNewton::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_pWorld = NewtonCreate();
        if (m_pWorld != nullptr)
        {
            NewtonWorldSetUserData(m_pWorld, (void *)this);
            int nDefaultID = NewtonMaterialGetDefaultGroupID(m_pWorld);
            NewtonMaterialSetCollisionCallback(m_pWorld, nDefaultID, nDefaultID, (void *)this, 
            [](const NewtonMaterial *pMaterial, const NewtonBody *pBody0, const NewtonBody *pBody1, int nThreadID) -> int
            {
                return 1;
            },
            [](const NewtonJoint *const pContactJoint, dFloat nFrameTime, int nThreadID) -> void
            {
                NewtonBody *pBody0 = NewtonJointGetBody0(pContactJoint);
                NewtonBody *pBody1 = NewtonJointGetBody1(pContactJoint);

                NewtonWorld *pWorld = NewtonBodyGetWorld(pBody0);
                CGEKComponentSystemNewton *pSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
                if (pSystem)
                {
                    GEKENTITYID nEntityID0 = GEKENTITYID(NewtonBodyGetUserData(pBody0));
                    GEKENTITYID nEntityID1 = GEKENTITYID(NewtonBodyGetUserData(pBody1));
                    if (nEntityID0 != GEKINVALIDENTITYID &&
                        nEntityID1 != GEKINVALIDENTITYID)
                    {
                        NewtonWorldCriticalSectionLock(pWorld, nThreadID);
                        for (void *pContact = NewtonContactJointGetFirstContact(pContactJoint); pContact; pContact = NewtonContactJointGetNextContact(pContactJoint, pContact))
                        {
                            NewtonMaterial *pMaterial = NewtonContactGetMaterial(pContact);
                            pSystem->OnCollisionContact(pMaterial, pBody0, nEntityID0, pBody1, nEntityID1);
                        }

                        NewtonWorldCriticalSectionUnlock(pWorld);
                    }
                }
            });

            //NewtonMaterialSetDefaultSoftness(m_pWorld, nDefaultID, nDefaultID, dFloat value);
            //NewtonMaterialSetDefaultElasticity(m_pWorld, nDefaultID, nDefaultID, dFloat elasticCoef);
            //NewtonMaterialSetDefaultCollidable(m_pWorld, nDefaultID, nDefaultID, 1);
            //NewtonMaterialSetDefaultFriction(m_pWorld, nDefaultID, nDefaultID, dFloat staticFriction, dFloat kineticFriction);

            hRetVal = S_OK;
        }
    }

    return hRetVal;
};

STDMETHODIMP_(void) CGEKComponentSystemNewton::Destroy(void)
{
    if (m_pSceneManager)
    {
        CGEKObservable::RemoveObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (m_pWorld != nullptr)
    {
        for (auto kPair : m_aCollisions)
        {
            NewtonDestroyCollision(kPair.second);
        }

        m_aBodies.clear();
        m_aCollisions.clear();
        m_aMaterials.clear();
        NewtonDestroy(m_pWorld);
        m_pWorld = nullptr;
    }
}

STDMETHODIMP CGEKComponentSystemNewton::OnLoadEnd(HRESULT hRetVal)
{
    if (FAILED(hRetVal))
    {
        OnFree();
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnFree(void)
{
    if (m_pWorld != nullptr)
    {
        for (auto kPair : m_aCollisions)
        {
            NewtonDestroyCollision(kPair.second);
        }

        m_aBodies.clear();
        m_aCollisions.clear();
        m_aMaterials.clear();
        NewtonDestroyAllBodies(m_pWorld);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnEntityDestroyed(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aBodies.find(nEntityID);
    if (pIterator != m_aBodies.end())
    {
        NewtonDestroyBody((*pIterator).second);
        m_aBodies.unsafe_erase(pIterator);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentAdded(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    if (nComponentID == GET_COMPONENT_ID(newton))
    {
        if (m_pSceneManager->HasComponent(nEntityID, GET_COMPONENT_ID(transform)))
        {
            auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            auto &kNewton = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID, GET_COMPONENT_ID(newton));
            if (!kNewton.shape.IsEmpty())
            {
                float4x4 nMatrix;
                nMatrix = kTransform.rotation;
                nMatrix.t = kTransform.position;
                NewtonCollision *pCollision = LoadCollision(kNewton.shape, kNewton.params);
                if (pCollision != nullptr)
                {
                    NewtonBody *pBody = NewtonCreateDynamicBody(m_pWorld, pCollision, nMatrix.data);
                    if (pBody != nullptr)
                    {
                        NewtonBodySetUserData(pBody, (void *)nEntityID);
                        NewtonBodySetMassProperties(pBody, kNewton.mass, pCollision);
                        NewtonBodySetTransformCallback(pBody, [](const NewtonBody *pBody, const dFloat *pMatrix, int nThreadID) -> void
                        {
                            REQUIRE_VOID_RETURN(pBody);
                            NewtonWorld *pWorld = NewtonBodyGetWorld(pBody);
                            if (pWorld)
                            {
                                CGEKComponentSystemNewton *pSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
                                if (pSystem)
                                {
                                    GEKENTITYID nEntityID = (GEKENTITYID)NewtonBodyGetUserData(pBody);
                                    pSystem->OnEntityTransformed(pBody, nEntityID, *(float4x4 *)pMatrix);
                                }
                            }
                        });

                        NewtonBodySetForceAndTorqueCallback(pBody, [](const NewtonBody *pBody, dFloat nFrameTime, int nThreadID) -> void
                        {
                            REQUIRE_VOID_RETURN(pBody);
                            NewtonWorld *pWorld = NewtonBodyGetWorld(pBody);
                            CGEKComponentSystemNewton *pSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
                            if (pSystem)
                            {
                                GEKENTITYID nEntityID = (GEKENTITYID)NewtonBodyGetUserData(pBody);
                                pSystem->OnSetForceAndTorque(pBody, nEntityID);
                            }
                        });

                        NewtonBodySetDestructorCallback(pBody, [](const NewtonBody *pBody) -> void
                        {
                            NewtonWorld *pWorld = NewtonBodyGetWorld(pBody);
                            CGEKComponentSystemNewton *pSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
                            if (pSystem)
                            {
                                GEKENTITYID nEntityID = (GEKENTITYID)NewtonBodyGetUserData(pBody);
                                auto pIterator = pSystem->m_aBodies.find(nEntityID);
                                if (pIterator != pSystem->m_aBodies.end())
                                {
                                    pSystem->m_aBodies.unsafe_erase(pIterator);
                                }
                            }
                        });

                        m_aBodies[nEntityID] = pBody;
                    }
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentRemoved(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(newton))
    {
        auto pIterator = m_aBodies.find(nEntityID);
        if (pIterator != m_aBodies.end())
        {
            NewtonDestroyBody((*pIterator).second);
            m_aBodies.unsafe_erase(pIterator);
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnUpdate(float nGameTime, float nFrameTime)
{
    NewtonUpdate(m_pWorld, nFrameTime);
}

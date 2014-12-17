#include "CGEKComponentNewton.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

#pragma comment(lib, "newton.lib")

REGISTER_COMPONENT(newton)
    REGISTER_COMPONENT_DEFAULT_VALUE(shape, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(params, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(mass, 0.0f)
    REGISTER_COMPONENT_SERIALIZE(newton)
        REGISTER_COMPONENT_SERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(newton)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass, StrToFloat)
END_REGISTER_COMPONENT(newton)

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemNewton)

int CGEKComponentSystemNewton::OnAABBOverlap(const NewtonMaterial *pMaterial, const NewtonBody *pBody0, const NewtonBody *pBody1, int nThreadID)
{
    return 1;
}

void CGEKComponentSystemNewton::ContactsProcess(const NewtonJoint *const pContactJoint, dFloat nFrameTime, int nThreadID)
{
/*
    NewtonBody *pBody0 = NewtonJointGetBody0(pContactJoint);
    NewtonBody *pBody1 = NewtonJointGetBody1(pContactJoint);
    NewtonWorld *pWorld = NewtonBodyGetWorld(pBody0);

    GEKENTITYID nEntityID0 = GEKENTITYID(NewtonBodyGetUserData(pBody0));
    GEKENTITYID nEntityID1 = GEKENTITYID(NewtonBodyGetUserData(pBody1));
    if(nEntityID0 != GEKINVALIDENTITYID && 
       nEntityID1 != GEKINVALIDENTITYID)
    {
    }

    NewtonWorldCriticalSectionLock(pWorld, nThreadID);
    for (void *pContact = NewtonContactJointGetFirstContact(pContactJoint); pContact; pContact = NewtonContactJointGetNextContact(pContactJoint, pContact))
    {
        float3 nPoint;
        float3 nNormal;
        NewtonMaterial *pMaterial = NewtonContactGetMaterial(pContact);
        NewtonMaterialGetContactPositionAndNormal(pMaterial, pBody0, nPoint.xyz, nNormal.xyz);
    }

    NewtonWorldCriticalSectionUnlock(pWorld);
*/
}

CGEKComponentSystemNewton::CGEKComponentSystemNewton(void)
    : m_pSceneManager(nullptr)
    , m_pWorld(nullptr)
{
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
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
    else if (_wcsicmp(pShape, L"convex_hull") == 0 ||
             _wcsicmp(pShape, L"tree") == 0)
    {
        GEKFUNCTION(L"Shape(%s), Params(%s)", pShape, pParams);

        std::vector<UINT8> aBuffer;
        HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.gek", pParams), aBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to Load Collision File failed: 0x%08X", hRetVal);
        if (SUCCEEDED(hRetVal))
        {
            UINT8 *pBuffer = &aBuffer[0];
            UINT32 nGEKX = *((UINT32 *)pBuffer);
            GEKRESULT(nGEKX == *(UINT32 *)"GEKX", L"Invalid Magic Header: %d", nGEKX);
            pBuffer += sizeof(UINT32);

            UINT16 nType = *((UINT16 *)pBuffer);
            GEKRESULT(nType == 0, L"Invalid Header Type: %d", nType);
            pBuffer += sizeof(UINT16);

            UINT16 nVersion = *((UINT16 *)pBuffer);
            GEKRESULT(nVersion == 2, L"Invalid Header Version: %d", nVersion);
            pBuffer += sizeof(UINT16);

            if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
            {
                aabb nAABB = *(aabb *)pBuffer;
                pBuffer += sizeof(aabb);

                UINT32 nNumMaterials = *((UINT32 *)pBuffer);
                GEKLOG(L"Number of Materials: %d", nNumMaterials);
                pBuffer += sizeof(UINT32);

                std::vector<MATERIAL> aMaterials(nNumMaterials);
                for (UINT32 nMaterial = 0; nMaterial < nNumMaterials; ++nMaterial)
                {
                    CStringA strMaterialUTF8 = pBuffer;
                    pBuffer += (strMaterialUTF8.GetLength() + 1);
                    CStringW strMaterial = CA2W(strMaterialUTF8, CP_UTF8);

                    MATERIAL &kMaterial = aMaterials[nMaterial];
                    kMaterial.m_nFirstVertex = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    kMaterial.m_nFirstIndex = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    kMaterial.m_nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);
                }

                UINT32 nNumVertices = *((UINT32 *)pBuffer);
                GEKLOG(L"Number of Vertices: %d", nNumVertices);
                pBuffer += sizeof(UINT32);

                float3 *pVertices = (float3 *)pBuffer;
                pBuffer += (sizeof(float3) * nNumVertices);
                pBuffer += (sizeof(float2) * nNumVertices);
                pBuffer += (sizeof(float3) * nNumVertices);

                UINT32 nNumIndices = *((UINT32 *)pBuffer);
                GEKLOG(L"Number of Indices: %d", nNumIndices);
                pBuffer += sizeof(UINT32);

                UINT16 *pIndices = (UINT16 *)pBuffer;

                if (_wcsicmp(pShape, L"convex_hull") == 0)
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
                        for (UINT32 nMaterial = 0; nMaterial < aMaterials.size(); nMaterial++)
                        {
                            MATERIAL &kMaterial = aMaterials[nMaterial];
                            for (UINT32 nIndex = 0; nIndex < kMaterial.m_nNumIndices; nIndex += 3)
                            {
                                float3 aFace[3] =
                                {
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 0]],
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 1]],
                                    pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 2]],
                                };

                                NewtonTreeCollisionAddFace(pCollision, 3, aFace[0].xyz, sizeof(float3), nMaterial);
                            }
                        }

                        NewtonTreeCollisionEndBuild(pCollision, false);
                    }
                }
            }
        }
    }

    if (pCollision)
    {
        m_aCollisions[strIdentity] = pCollision;
    }

    return pCollision;
}

void CGEKComponentSystemNewton::OnEntityUpdated(const NewtonBody *pBody, const GEKENTITYID &nEntityID)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    float4x4 nMatrix;
    NewtonBodyGetMatrix(pBody, nMatrix.data);
    auto &kNewton = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID, L"newton");
    float3 nGravity = (m_pSceneManager->GetGravity(nMatrix.t) * kNewton.mass);
    NewtonBodyAddForce(pBody, nGravity.xyz);
}

void CGEKComponentSystemNewton::OnEntityTransformed(const NewtonBody *pBody, const GEKENTITYID &nEntityID, const float4x4 &nMatrix)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, L"transform");
    kTransform.position = nMatrix.t;
    kTransform.rotation = nMatrix;
}

STDMETHODIMP CGEKComponentSystemNewton::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
    if (m_pSceneManager)
    {
        hRetVal = CGEKObservable::AddObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_pWorld = NewtonCreate();
        if (m_pWorld != nullptr)
        {
            NewtonWorldSetUserData(m_pWorld, (void *)this);
            int nDefaultID = NewtonMaterialGetDefaultGroupID(m_pWorld);
            NewtonMaterialSetCollisionCallback(m_pWorld, nDefaultID, nDefaultID, (void *)this, OnAABBOverlap, ContactsProcess);
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

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentAdded(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    if (_wcsicmp(pComponent, L"newton") == 0)
    {
        if (m_pSceneManager->HasComponent(nEntityID, L"transform"))
        {
            auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, L"transform");
            auto &kNewton = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID, L"newton");
            if (!kNewton.shape.IsEmpty())
            {
                float4x4 nMatrix;
                nMatrix = kTransform.rotation;
                nMatrix.t = kTransform.position;
                NewtonCollision *pCollision = LoadCollision(kNewton.shape, kNewton.params);
                if (pCollision != nullptr)
                {
                    NewtonBody *pBody = NewtonCreateDynamicBody(m_pWorld, pCollision, nMatrix.data);
                    GEKRESULT(pBody, L"Call to NewtonCreateDynamicBody failed to allocate instance");
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
                            if (pWorld)
                            {
                                CGEKComponentSystemNewton *pSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
                                if (pSystem)
                                {
                                    GEKENTITYID nEntityID = (GEKENTITYID)NewtonBodyGetUserData(pBody);
                                    pSystem->OnEntityUpdated(pBody, nEntityID);
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

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentRemoved(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    if (_wcsicmp(pComponent, L"newton") == 0)
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

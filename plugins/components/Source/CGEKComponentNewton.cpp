#include "CGEKComponentNewton.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

#pragma comment(lib, "newton.lib")

BEGIN_INTERFACE_LIST(CGEKComponentNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentNewton)

CGEKComponentNewton::CGEKComponentNewton(void)
{
}

CGEKComponentNewton::~CGEKComponentNewton(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentNewton::GetName(void) const
{
    return L"newton";
};

STDMETHODIMP_(void) CGEKComponentNewton::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentNewton::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentNewton::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentNewton::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentNewton::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"shape", (*pIterator).second.m_strShape.GetString());
        OnProperty(L"params", (*pIterator).second.m_strParams.GetString());
        OnProperty(L"mass", (*pIterator).second.m_nMass);
    }
}

STDMETHODIMP_(bool) CGEKComponentNewton::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"shape") == 0)
        {
            kValue = (*pIterator).second.m_strShape.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"params") == 0)
        {
            kValue = (*pIterator).second.m_strParams.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"mass") == 0)
        {
            kValue = (*pIterator).second.m_nMass;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentNewton::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"shape") == 0)
        {
            (*pIterator).second.m_strShape = kValue.GetRawString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"params") == 0)
        {
            (*pIterator).second.m_strParams = kValue.GetRawString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"mass") == 0)
        {
            (*pIterator).second.m_nMass = kValue.GetFloat();
            bReturn = true;
        }
    }

    return bReturn;
}

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
    NewtonBody *pBody0 = NewtonJointGetBody0(pContactJoint);
    NewtonBody *pBody1 = NewtonJointGetBody1(pContactJoint);
    NewtonWorld *pWorld = NewtonBodyGetWorld(pBody0);

    CGEKComponentSystemNewton *pNewtonSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
    if (pNewtonSystem != nullptr)
    {
        CGEKComponentNewton *pComponent0 = static_cast<CGEKComponentNewton *>(NewtonBodyGetUserData(pBody0));
        CGEKComponentNewton *pComponent1 = static_cast<CGEKComponentNewton *>(NewtonBodyGetUserData(pBody1));
        if (pComponent0 != nullptr && pComponent1 != nullptr)
        {
/*          
            CComPtr<IUnknown> spComponent0;
            CComPtr<IUnknown> spComponent1;
            pComponent0->QueryInterface(IID_PPV_ARGS(&spComponent0));
            pComponent1->QueryInterface(IID_PPV_ARGS(&spComponent1));
            pComponent0->GetEntity()->OnEvent(L"collision", (IUnknown *)spComponent1);
            pComponent1->GetEntity()->OnEvent(L"collision", (IUnknown *)spComponent0);
*/
        }
        else if (pComponent0 != nullptr)
        {
/*          
            pComponent0->GetEntity()->OnEvent(L"collision");
*/
        }
        else if (pComponent1 != nullptr)
        {
/*          
            pComponent1->GetEntity()->OnEvent(L"collision");
*/
        }
    }
/*
    NewtonWorld *pWorld = NewtonBodyGetWorld(pBody0);
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
    : m_pWorld(nullptr)
{
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
}

NewtonCollision *CGEKComponentSystemNewton::LoadCollision(LPCWSTR pShape, LPCWSTR pParams)
{
    float4x4 nIdentityMatrix;
    NewtonCollision *pCollision = nullptr;
    if (_wcsicmp(pShape, L"cube") == 0)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateBox(m_pWorld, nSize.x, nSize.y, nSize.z, (int)L"cube", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"sphere") == 0)
    {
        float nSize = StrToFloat(pParams);
        pCollision = NewtonCreateSphere(m_pWorld, nSize, (int)L"sphere", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"cone") == 0)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCone(m_pWorld, nSize.x, nSize.y, (int)L"cone", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"capsule") == 0)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCapsule(m_pWorld, nSize.x, nSize.y, (int)L"capsule", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"cylinder") == 0)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCylinder(m_pWorld, nSize.x, nSize.y, (int)L"cylinder", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"tapered_capsule") == 0)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateTaperedCapsule(m_pWorld, nSize.x, nSize.y, nSize.z, (int)L"tapered_capsule", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"tapered_cylinder") == 0)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateTaperedCylinder(m_pWorld, nSize.x, nSize.y, nSize.z, (int)L"tapered_cylinder", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"chamfer_cylinder") == 0)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateChamferCylinder(m_pWorld, nSize.x, nSize.y, (int)L"chamfer_cylinder", nIdentityMatrix.data);
    }
    else if (_wcsicmp(pShape, L"convex_hull") == 0)
    {
        IGEKModelManager *pModelManager = GetContext()->GetCachedClass<IGEKModelManager>(CLSID_GEKModelManager);
        if (pModelManager != nullptr)
        {
            CComPtr<IGEKCollision> spCollision;
            pModelManager->LoadCollision(pParams, L"", &spCollision);
            if (spCollision)
            {
                std::vector<float3> aCloud(spCollision->GetNumIndices());
                for (UINT32 nIndex = 0; nIndex < spCollision->GetNumIndices(); ++nIndex)
                {
                    aCloud[nIndex] = spCollision->GetVertices()[spCollision->GetIndices()[nIndex]];
                }

                pCollision = NewtonCreateConvexHull(m_pWorld, aCloud.size(), aCloud[0].xyz, sizeof(float3), 0.025f, (int)L"convex_hull", nIdentityMatrix.data);
            }
        }
    }
    else if (_wcsicmp(pShape, L"tree") == 0)
    {
        IGEKModelManager *pModelManager = GetContext()->GetCachedClass<IGEKModelManager>(CLSID_GEKModelManager);
        if (pModelManager != nullptr)
        {
            CComPtr<IGEKCollision> spCollision;
            pModelManager->LoadCollision(pParams, L"", &spCollision);
            if (spCollision)
            {
                pCollision = NewtonCreateTreeCollision(m_pWorld, (int)L"tree");
                if (pCollision != nullptr)
                {
                    NewtonTreeCollisionBeginBuild(pCollision);
                    for (UINT32 nIndex = 0; nIndex < spCollision->GetNumIndices(); nIndex += 3)
                    {
                        float3 aFace[3] =
                        {
                            spCollision->GetVertices()[spCollision->GetIndices()[nIndex + 0]],
                            spCollision->GetVertices()[spCollision->GetIndices()[nIndex + 1]],
                            spCollision->GetVertices()[spCollision->GetIndices()[nIndex + 2]],
                        };

                        NewtonTreeCollisionAddFace(pCollision, 3, aFace[0].xyz, sizeof(float3), 0);
                    }

                    NewtonTreeCollisionEndBuild(pCollision, true);
                }
            }
        }
    }

    return pCollision;
}

void CGEKComponentSystemNewton::OnEntityUpdated(const NewtonBody *pBody, const GEKENTITYID &nEntityID)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
    if (pSceneManager != nullptr)
    {
        GEKVALUE kMass;
        pSceneManager->GetProperty(nEntityID, L"newton", L"mass", kMass);

        float4x4 nMatrix;
        NewtonBodyGetMatrix(pBody, nMatrix.data);

        float3 nGravity = (pSceneManager->GetGravity(nMatrix.t) * kMass.GetFloat());
        NewtonBodyAddForce(pBody, nGravity.xyz);
    }
}

void CGEKComponentSystemNewton::OnEntityTransformed(const NewtonBody *pBody, const GEKENTITYID &nEntityID, const float4x4 &nMatrix)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
    if (pSceneManager != nullptr)
    {
        pSceneManager->SetProperty(nEntityID, L"transform", L"position", nMatrix.t);
        pSceneManager->SetProperty(nEntityID, L"transform", L"rotation", quaternion(nMatrix));
    }
}

STDMETHODIMP CGEKComponentSystemNewton::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pWorld = NewtonCreate();
    if (m_pWorld != nullptr)
    {
        NewtonWorldSetUserData(m_pWorld, (void *)this);
        int nDefaultID = NewtonMaterialGetDefaultGroupID(m_pWorld);
        NewtonMaterialSetCollisionCallback(m_pWorld, nDefaultID, nDefaultID, (void *)this, OnAABBOverlap, ContactsProcess);
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    }

    return hRetVal;
};

STDMETHODIMP_(void) CGEKComponentSystemNewton::Destroy(void)
{
    m_aBodies.clear();
    m_aCollisions.clear();
    if (m_pWorld != nullptr)
    {
        NewtonDestroy(m_pWorld);
        m_pWorld = nullptr;
    }

    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
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
    m_aBodies.clear();
    if (m_pWorld != nullptr)
    {
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
    if (_wcsicmp(pComponent, L"newton") == 0)
    {
        IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
        if (pSceneManager != nullptr)
        {
            if (pSceneManager->HasComponent(nEntityID, L"transform"))
            {
                GEKVALUE kShape;
                pSceneManager->GetProperty(nEntityID, L"newton", L"shape", kShape);
                if (!kShape.GetString().IsEmpty())
                {
                    GEKVALUE kMass;
                    GEKVALUE kParams;
                    pSceneManager->GetProperty(nEntityID, L"newton", L"mass", kMass);
                    pSceneManager->GetProperty(nEntityID, L"newton", L"params", kParams);

                    GEKVALUE kPosition;
                    GEKVALUE kRotation;
                    pSceneManager->GetProperty(nEntityID, L"transform", L"position", kPosition);
                    pSceneManager->GetProperty(nEntityID, L"transform", L"rotation", kRotation);

                    float4x4 nMatrix;
                    nMatrix = kRotation.GetQuaternion();
                    nMatrix.t = kPosition.GetFloat3();

                    NewtonCollision *pCollision = LoadCollision(kShape.GetRawString(), kParams.GetRawString());
                    if (pCollision != nullptr)
                    {
                        NewtonBody *pBody = NewtonCreateDynamicBody(m_pWorld, pCollision, nMatrix.data);
                        GEKRESULT(pBody, L"Call to NewtonCreateDynamicBody failed to allocate instance");
                        if (pBody != nullptr)
                        {
                            NewtonBodySetUserData(pBody, (void *)nEntityID);
                            NewtonBodySetMassProperties(pBody, kMass.GetFloat(), pCollision);
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

                        NewtonDestroyCollision(pCollision);
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

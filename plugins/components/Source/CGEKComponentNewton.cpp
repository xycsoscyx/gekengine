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
            (*pIterator).second.m_strShape = kValue.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"params") == 0)
        {
            (*pIterator).second.m_strParams = kValue.GetString();
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

STDMETHODIMP CGEKComponentSystemNewton::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pWorld = NewtonCreate();
    if (m_pWorld != nullptr)
    {
        NewtonWorldSetUserData(m_pWorld, (void *)this);
        int nDefaultID = NewtonMaterialGetDefaultGroupID(m_pWorld);
        NewtonMaterialSetCollisionCallback(m_pWorld, nDefaultID, nDefaultID, (void *)this, OnAABBOverlap, ContactsProcess);
        hRetVal = S_OK;
    }

    if (SUCCEEDED(hRetVal))
    {
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
/*
STDMETHODIMP CGEKComponentNewton::OnEntityCreated(void)
{
    HRESULT hRetVal = E_FAIL;
    if (!m_strShape.IsEmpty())
    {
        IGEKComponent *pTransform = GetEntity()->GetComponent(L"transform");
        if (pTransform != nullptr)
        {
            IGEKNewtonSystem *pNewtonSystem = GetContext()->GetCachedClass<IGEKNewtonSystem>(CLSID_GEKComponentSystemNewton);
            if (pNewtonSystem != nullptr)
            {
                NewtonCollision *pCollision = pNewtonSystem->LoadCollision(m_strShape, m_strParams);
                if (pCollision != nullptr)
                {
                    GEKVALUE kPosition;
                    GEKVALUE kRotation;
                    pTransform->GetProperty(L"position", kPosition);
                    pTransform->GetProperty(L"rotation", kRotation);

                    float4x4 nMatrix;
                    nMatrix = kRotation.GetQuaternion();
                    nMatrix.t = kPosition.GetFloat3();

                    m_pBody = NewtonCreateDynamicBody(pNewtonSystem->GetWorld(), pCollision, nMatrix.data);
                    GEKRESULT(m_pBody, L"Call to NewtonCreateDynamicBody failed to allocate instance");
                    if (m_pBody != nullptr)
                    {
                        NewtonBodySetMassProperties(m_pBody, m_nMass, pCollision);

                        NewtonBodySetUserData(m_pBody, (void *)this);
                        NewtonBodySetForceAndTorqueCallback(m_pBody, [](const NewtonBody *pBody, dFloat nFrameTime, int nThreadID) -> void
                        {
                            REQUIRE_VOID_RETURN(pBody);
                            CGEKComponentNewton *pComponent = (CGEKComponentNewton *)NewtonBodyGetUserData(pBody);
                            if (pComponent != nullptr)
                            {
                                IGEKSceneManager *pSceneManager = pComponent->GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
                                if (pSceneManager != nullptr)
                                {
                                    float4x4 nMatrix;
                                    NewtonBodyGetMatrix(pBody, nMatrix.data);
                                    float3 nGravity = (pSceneManager->GetGravity(nMatrix.t) * pComponent->m_nMass);
                                    NewtonBodyAddForce(pBody, nGravity.xyz);
                                }
                            }
                        });

                        hRetVal = S_OK;
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnUpdate(float nGameTime, float nFrameTime)
{
    NewtonUpdate(m_pWorld, nFrameTime);
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentNewton>>::value_type &kPair) -> void
    {
        IGEKComponent *pTransform = kPair.first->GetComponent(L"transform");
        if (pTransform && kPair.second->m_pBody)
        {
            float4x4 nMatrix;
            NewtonBodyGetMatrix(kPair.second->m_pBody, nMatrix.data);
            pTransform->SetProperty(L"position", nMatrix.t);
            pTransform->SetProperty(L"rotation", quaternion(nMatrix));
        }
    });
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnFree(void)
{
    m_aComponents.clear();
    m_aCollisions.clear();
    if (m_pWorld != nullptr)
    {
        NewtonDestroyAllBodies(m_pWorld);
    }
}

STDMETHODIMP CGEKComponentSystemNewton::Create(const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKComponentNewton> spComponent(new CGEKComponentNewton(GetContext(), pEntity));
    GEKRESULT(spComponent, L"Unable to allocate new newton component instance");
    if (spComponent)
    {
        hRetVal = spComponent->QueryInterface(IID_PPV_ARGS(ppComponent));
        if (SUCCEEDED(hRetVal))
        {
            kComponentNode.ListAttributes([&spComponent](LPCWSTR pName, LPCWSTR pValue) -> void
            {
                spComponent->SetProperty(pName, pValue);
            });

            m_aComponents[pEntity] = spComponent;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemNewton::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(pEntity);
    if (pIterator != m_aComponents.end())
    {
        if (((*pIterator).second)->m_pBody)
        {
            NewtonDestroyBody(((*pIterator).second)->m_pBody);
        }

        m_aComponents.unsafe_erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(NewtonWorld *) CGEKComponentSystemNewton::GetWorld(void)
{
    return m_pWorld;
}

static GEKHASH gs_nShapeCube(L"cube");
static GEKHASH gs_nShapeSphere(L"sphere");
static GEKHASH gs_nShapeCone(L"cone");
static GEKHASH gs_nShapeCapsule(L"capsule");
static GEKHASH gs_nShapeCylinder(L"cylinder");
static GEKHASH gs_nShapeTaperedCapsule(L"tapered_capsule");
static GEKHASH gs_nShapeTaperedCylinder(L"tapered_cylinder");
static GEKHASH gs_nShapeChamferCylinder(L"chamfer_cylinder");
static GEKHASH gs_nShapeConvexHull(L"convex_hull");
static GEKHASH gs_nShapeTree(L"tree");
STDMETHODIMP_(NewtonCollision *) CGEKComponentSystemNewton::LoadCollision(LPCWSTR pShape, LPCWSTR pParams)
{
    GEKHASH nFullHash(FormatString(L"%s|%s", pShape, pParams));
    auto pIterator = m_aCollisions.find(nFullHash);
    if (pIterator != m_aCollisions.end())
    {
        return ((*pIterator).second);
    }

    GEKHASH nHash(pShape);
    float4x4 nIdentityMatrix;
    NewtonCollision *pCollision = nullptr;
    if (nHash == gs_nShapeCube)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateBox(m_pWorld, nSize.x, nSize.y, nSize.z, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeSphere)
    {
        float nSize = StrToFloat(pParams);
        pCollision = NewtonCreateSphere(m_pWorld, nSize, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeCone)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCone(m_pWorld, nSize.x, nSize.y, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeCapsule)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCapsule(m_pWorld, nSize.x, nSize.y, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeCylinder)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateCylinder(m_pWorld, nSize.x, nSize.y, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeTaperedCapsule)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateTaperedCapsule(m_pWorld, nSize.x, nSize.y, nSize.z, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeTaperedCylinder)
    {
        float3 nSize = StrToFloat3(pParams);
        pCollision = NewtonCreateTaperedCylinder(m_pWorld, nSize.x, nSize.y, nSize.z, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeChamferCylinder)
    {
        float2 nSize = StrToFloat2(pParams);
        pCollision = NewtonCreateChamferCylinder(m_pWorld, nSize.x, nSize.y, nHash.GetHash(), nIdentityMatrix.data);
    }
    else if (nHash == gs_nShapeConvexHull)
    {
        IGEKModelManager *pModelManager = GetContext()->GetCachedClass<IGEKModelManager>(CLSID_GEKRenderManager);
        if (pModelManager != nullptr)
        {
            CComPtr<IGEKCollision> spCollision;
            pModelManager->LoadCollision(pParams, L"", &spCollision);
            if (spCollision)
            {
                std::vector<float3> aCloud(spCollision->GetNumIndices());
                for (UINT32 nIndex = 0; nIndex < spCollision->GetNumIndices(); nIndex++)
                {
                    aCloud[nIndex] = spCollision->GetVertices()[spCollision->GetIndices()[nIndex]];
                }

                pCollision = NewtonCreateConvexHull(m_pWorld, aCloud.size(), aCloud[0].xyz, sizeof(float3), 0.025f, nHash.GetHash(), nIdentityMatrix.data);
            }
        }
    }
    else if (nHash == gs_nShapeTree)
    {
        IGEKModelManager *pModelManager = GetContext()->GetCachedClass<IGEKModelManager>(CLSID_GEKRenderManager);
        if (pModelManager != nullptr)
        {
            CComPtr<IGEKCollision> spCollision;
            pModelManager->LoadCollision(pParams, L"", &spCollision);
            if (spCollision)
            {
                pCollision = NewtonCreateTreeCollision(m_pWorld, nHash.GetHash());
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

    if (pCollision != nullptr)
    {
        m_aCollisions[nFullHash] = pCollision;
    }

    return pCollision;
}
*/
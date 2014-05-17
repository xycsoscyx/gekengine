#include "CGEKComponentNewton.h"
#include <ppl.h>

#pragma comment(lib, "newton.lib")

BEGIN_INTERFACE_LIST(CGEKComponentNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentNewton::CGEKComponentNewton(IGEKEntity *pEntity, CGEKComponentSystemNewton *pSystem)
    : CGEKComponent(pEntity)
    , m_pSystem(pSystem)
    , m_pBody(nullptr)
    , m_nMass(0.0f)
{
}

CGEKComponentNewton::~CGEKComponentNewton(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentNewton::GetType(void) const
{
    return L"newton";
}

STDMETHODIMP_(void) CGEKComponentNewton::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"shape", m_strShape.GetString());
    OnProperty(L"params", m_strParams.GetString());
    OnProperty(L"mass", m_nMass);
}

static GEKHASH gs_nShape(L"shape");
static GEKHASH gs_nParams(L"params");
static GEKHASH gs_nMass(L"mass");
STDMETHODIMP_(bool) CGEKComponentNewton::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nShape)
    {
        kValue = m_strShape.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        kValue = m_strParams.GetString();
        return true;
    }
    else if (nHash == gs_nMass)
    {
        kValue = m_nMass;
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentNewton::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nShape)
    {
        m_strShape = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        m_strParams = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nMass)
    {
        m_nMass = kValue.GetFloat();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentNewton::OnEntityCreated(void)
{
    HRESULT hRetVal = E_FAIL;
    if (!m_strShape.IsEmpty())
    {
        IGEKComponent *pTransform = GetEntity()->GetComponent(L"transform");
        if (pTransform)
        {
            NewtonCollision *pCollision = m_pSystem->LoadCollision(m_strShape, m_strParams);
            if (pCollision)
            {
                GEKVALUE kPosition;
                GEKVALUE kRotation;
                pTransform->GetProperty(L"position", kPosition);
                pTransform->GetProperty(L"rotation", kRotation);

                float4x4 nMatrix;
                nMatrix = kRotation.GetQuaternion();
                nMatrix.t = kPosition.GetFloat3();

                m_pBody = NewtonCreateDynamicBody(m_pSystem->GetWorld(), pCollision, nMatrix.data);
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
                            float4x4 nMatrix;
                            NewtonBodyGetMatrix(pBody, nMatrix.data);
                            float3 nGravity = (pComponent->GetSceneManager()->GetGravity(nMatrix.t) * pComponent->m_nMass);
                            NewtonBodyAddForce(pBody, nGravity.xyz);
                        }
                    });

                    hRetVal = S_OK;
                }
            }
        }
    }

    return hRetVal;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKModelManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemNewton)

CGEKComponentSystemNewton::CGEKComponentSystemNewton(void)
    : m_pWorld(nullptr)
{
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
}

NewtonWorld *CGEKComponentSystemNewton::GetWorld(void)
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
NewtonCollision *CGEKComponentSystemNewton::LoadCollision(LPCWSTR pShape, LPCWSTR pParams)
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
        CComPtr<IGEKCollision> spCollision;
        GetModelManager()->LoadCollision(pParams, L"", &spCollision);
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
    else if (nHash == gs_nShapeTree)
    {
        CComPtr<IGEKCollision> spCollision;
        GetModelManager()->LoadCollision(pParams, L"", &spCollision);
        if (spCollision)
        {
            pCollision = NewtonCreateTreeCollision(m_pWorld, nHash.GetHash());
            if (pCollision)
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
    
    if (pCollision)
    {
        m_aCollisions[nFullHash] = pCollision;
    }

    return pCollision;
}

int GEKNewtonOnAABBOverlap(const NewtonMaterial *pMaterial, const NewtonBody *pBody0, const NewtonBody *pBody1, int nThreadID)
{
    return 1;
}

void GEKNewtonContactsProcess(const NewtonJoint *const pContactJoint, dFloat nFrameTime, int nThreadID)
{
    NewtonBody *pBody0 = NewtonJointGetBody0(pContactJoint);
    NewtonBody *pBody1 = NewtonJointGetBody1(pContactJoint);
    NewtonWorld *pWorld = NewtonBodyGetWorld(pBody0);

    CGEKComponentSystemNewton *pNewtonSystem = (CGEKComponentSystemNewton *)NewtonWorldGetUserData(pWorld);
    if (pNewtonSystem)
    {
        CGEKComponentNewton *pComponent0 = static_cast<CGEKComponentNewton *>(NewtonBodyGetUserData(pBody0));
        CGEKComponentNewton *pComponent1 = static_cast<CGEKComponentNewton *>(NewtonBodyGetUserData(pBody1));
        if (pComponent0 && pComponent1)
        {
            CComPtr<IUnknown> spComponent0;
            CComPtr<IUnknown> spComponent1;
            pComponent0->QueryInterface(IID_PPV_ARGS(&spComponent0));
            pComponent1->QueryInterface(IID_PPV_ARGS(&spComponent1));
            pComponent0->GetEntity()->OnEvent(L"collision", (IUnknown *)spComponent1);
            pComponent1->GetEntity()->OnEvent(L"collision", (IUnknown *)spComponent0);
        }
        else if (pComponent0)
        {
            pComponent0->GetEntity()->OnEvent(L"collision");
        }
        else if (pComponent1)
        {
            pComponent1->GetEntity()->OnEvent(L"collision");
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

STDMETHODIMP CGEKComponentSystemNewton::Initialize(void)
{
    m_pWorld = NewtonCreate();
    NewtonWorldSetUserData(m_pWorld, (void *)this);
    int nDefaultID = NewtonMaterialGetDefaultGroupID(m_pWorld);
    NewtonMaterialSetCollisionCallback(m_pWorld, nDefaultID, nDefaultID, (void *)this, GEKNewtonOnAABBOverlap, GEKNewtonContactsProcess);
    return CGEKObservable::AddObserver(GetSceneManager(), (IGEKSceneObserver *)this);
};

STDMETHODIMP_(void) CGEKComponentSystemNewton::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetSceneManager(), (IGEKSceneObserver *)this);

    Clear();
    if (m_pWorld != nullptr)
    {
        NewtonDestroy(m_pWorld);
        m_pWorld = nullptr;
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::Clear(void)
{
    NewtonDestroyAllBodies(m_pWorld);
    m_aComponents.clear();
    m_aCollisions.clear();
}

STDMETHODIMP CGEKComponentSystemNewton::Create(const CLibXMLNode &kEntityNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kEntityNode.HasAttribute(L"type") && kEntityNode.GetAttribute(L"type").CompareNoCase(L"newton") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentNewton> spComponent(new CGEKComponentNewton(pEntity, this));
        if (spComponent)
        {
            CComPtr<IUnknown> spComponentUnknown;
            spComponent->QueryInterface(IID_PPV_ARGS(&spComponentUnknown));
            if (spComponentUnknown)
            {
                GetContext()->RegisterInstance(spComponentUnknown);
            }

            hRetVal = spComponent->QueryInterface(IID_PPV_ARGS(ppComponent));
            if (SUCCEEDED(hRetVal))
            {
                kEntityNode.ListAttributes([&spComponent] (LPCWSTR pName, LPCWSTR pValue) -> void
                {
                    spComponent->SetProperty(pName, pValue);
                } );

                m_aComponents[pEntity] = spComponent;
            }
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

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnLoadBegin(void)
{
}

STDMETHODIMP CGEKComponentSystemNewton::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
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

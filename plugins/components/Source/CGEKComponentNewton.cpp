#include "CGEKComponentNewton.h"
#include <ppl.h>

#pragma comment(lib, "newton.lib")

BEGIN_INTERFACE_LIST(CGEKComponentNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentNewton::CGEKComponentNewton(IGEKEntity *pEntity, NewtonWorld *pWorld)
    : CGEKComponent(pEntity)
    , m_pWorld(pWorld)
    , m_pBody(nullptr)
    , m_nMass(0.0f)
{
}

CGEKComponentNewton::~CGEKComponentNewton(void)
{
}

NewtonBody *CGEKComponentNewton::GetBody(void) const
{
    return m_pBody;
}

float CGEKComponentNewton::GetMass(void) const
{
    return m_nMass;
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
            float4x4 nIdentityMatrix;
            NewtonCollision *pCollision = nullptr;
            if (m_strShape.CompareNoCase(L"cube") == 0)
            {
                float3 nSize = StrToFloat3(m_strParams);
                pCollision = NewtonCreateBox(m_pWorld, nSize.x, nSize.y, nSize.z, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"sphere") == 0)
            {
                float nSize = StrToFloat(m_strParams);
                pCollision = NewtonCreateSphere(m_pWorld, nSize, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"cone") == 0)
            {
                float2 nSize = StrToFloat2(m_strParams);
                pCollision = NewtonCreateCone(m_pWorld, nSize.x, nSize.y, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"capsule") == 0)
            {
                float2 nSize = StrToFloat2(m_strParams);
                pCollision = NewtonCreateCapsule(m_pWorld, nSize.x, nSize.y, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"cylinder") == 0)
            {
                float2 nSize = StrToFloat2(m_strParams);
                pCollision = NewtonCreateCylinder(m_pWorld, nSize.x, nSize.y, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"tapered_capsule") == 0)
            {
                float3 nSize = StrToFloat3(m_strParams);
                pCollision = NewtonCreateTaperedCapsule(m_pWorld, nSize.x, nSize.y, nSize.z, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"tapered_cylinder") == 0)
            {
                float3 nSize = StrToFloat3(m_strParams);
                pCollision = NewtonCreateTaperedCylinder(m_pWorld, nSize.x, nSize.y, nSize.z, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"chamfer_cylinder") == 0)
            {
                float2 nSize = StrToFloat2(m_strParams);
                pCollision = NewtonCreateChamferCylinder(m_pWorld, nSize.x, nSize.y, int(this), nIdentityMatrix.data);
            }
            else if (m_strShape.CompareNoCase(L"convex_hull") == 0)
            {
                //pCollision = NewtonCreateConvexHull(pWorld, StrToFloat(m_strParams), int(this), nIdentityMatrix.data);
                //NEWTON_API NewtonCollision* NewtonCreateConvexHull (const NewtonWorld* const newtonWorld, int count, const dFloat* const vertexCloud, int strideInBytes, dFloat tolerance, int shapeID, const dFloat* const offsetMatrix);
            }

            if (pCollision)
            {
                GEKVALUE kPosition;
                GEKVALUE kRotation;
                pTransform->GetProperty(L"position", kPosition);
                pTransform->GetProperty(L"rotation", kRotation);

                float4x4 nMatrix;
                nMatrix = kRotation.GetQuaternion();
                nMatrix.t = kPosition.GetFloat3();

                m_pBody = NewtonCreateDynamicBody(m_pWorld, pCollision, nMatrix.data);
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
                            float3 nGravity = (pComponent->GetSceneManager()->GetGravity(nMatrix.t) * pComponent->GetMass());
                            NewtonBodyAddForce(pBody, nGravity.xyz);
                        }
                    });

                    hRetVal = S_OK;
                }

                NewtonDestroyCollision(pCollision);
            }
        }
    }

    return hRetVal;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
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
}

STDMETHODIMP CGEKComponentSystemNewton::Create(const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kNode.HasAttribute(L"type") && kNode.GetAttribute(L"type").CompareNoCase(L"newton") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentNewton> spComponent(new CGEKComponentNewton(pEntity, m_pWorld));
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
                kNode.ListAttributes([&spComponent] (LPCWSTR pName, LPCWSTR pValue) -> void
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
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentNewton>>::value_type &kPair) -> bool
    {
        return (kPair.first == pEntity);
    });

    if (pIterator != m_aComponents.end())
    {
        if (((*pIterator).second)->GetBody())
        {
            NewtonDestroyBody(((*pIterator).second)->GetBody());
        }

        m_aComponents.erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnLoadBegin(void)
{
    REQUIRE_VOID_RETURN(m_pWorld);
    
    m_pStatic = NewtonCreateTreeCollision(m_pWorld, int(GetTickCount()));
    if (m_pStatic)
    {
        NewtonTreeCollisionBeginBuild(m_pStatic);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnStaticFace(float3 *pFace, IUnknown *pMaterial)
{
    REQUIRE_VOID_RETURN(m_pStatic);
    NewtonTreeCollisionAddFace(m_pStatic, 3, &pFace->x, sizeof(float3), int(pMaterial));
}

STDMETHODIMP CGEKComponentSystemNewton::OnLoadEnd(HRESULT hRetVal)
{
    REQUIRE_RETURN(m_pStatic, E_INVALID);
    REQUIRE_RETURN(m_pWorld, E_INVALID);

    if (SUCCEEDED(hRetVal))
    {
        NewtonTreeCollisionEndBuild(m_pStatic, 1);

        float4x4 nMatrix;
        NewtonBody *pBody = NewtonCreateDynamicBody(m_pWorld, m_pStatic, nMatrix.data);
        if (pBody != nullptr)
        {
            hRetVal = S_OK;
        }
    }

    NewtonDestroyCollision(m_pStatic);
    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnUpdate(float nGameTime, float nFrameTime)
{
    NewtonUpdate(m_pWorld, nFrameTime);
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentNewton>>::value_type &kPair) -> void
    {
        IGEKComponent *pTransform = kPair.first->GetComponent(L"transform");
        if (pTransform && kPair.second->GetBody())
        {
            float4x4 nMatrix;
            NewtonBodyGetMatrix(kPair.second->GetBody(), nMatrix.data);
            pTransform->SetProperty(L"position", nMatrix.t);
            pTransform->SetProperty(L"rotation", quaternion(nMatrix));
        }
    });
}

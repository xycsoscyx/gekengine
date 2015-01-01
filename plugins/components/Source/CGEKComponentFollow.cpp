#include "CGEKComponentFollow.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

REGISTER_COMPONENT(follow)
    REGISTER_COMPONENT_DEFAULT_VALUE(target, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(offset, float3(0.0f, 0.0f, 0.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(rotation, quaternion(0.0f, 0.0f, 0.0f, 1.0f))
    REGISTER_COMPONENT_SERIALIZE(follow)
        REGISTER_COMPONENT_SERIALIZE_VALUE(target, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(offset, StrFromFloat3)
        REGISTER_COMPONENT_SERIALIZE_VALUE(rotation, StrFromQuaternion)
    REGISTER_COMPONENT_DESERIALIZE(follow)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(target, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(offset, StrToFloat3)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(rotation, StrToQuaternion)
END_REGISTER_COMPONENT(follow)

BEGIN_INTERFACE_LIST(CGEKComponentSystemFollow)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemFollow)

CGEKComponentSystemFollow::CGEKComponentSystemFollow(void)
    : m_pSceneManager(nullptr)
{
}

CGEKComponentSystemFollow::~CGEKComponentSystemFollow(void)
{
}

STDMETHODIMP CGEKComponentSystemFollow::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
    if (m_pSceneManager)
    {
        hRetVal = CGEKObservable::AddObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
    }

    return hRetVal;
};

STDMETHODIMP_(void) CGEKComponentSystemFollow::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
    if (m_pSceneManager)
    {
        CGEKObservable::RemoveObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFollow::OnUpdateEnd(float nGameTime, float nFrameTime)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"follow" }, [&](const GEKENTITYID &nEntityID)->void
    {
        auto &kFollow = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(follow)>(nEntityID, L"follow");

        GEKENTITYID nTargetID = GEKINVALIDENTITYID;
        if (SUCCEEDED(m_pSceneManager->GetNamedEntity(kFollow.target, &nTargetID)))
        {
            auto &kTargetTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nTargetID, L"transform");

            kFollow.rotation = kFollow.rotation.Slerp(kTargetTransform.rotation, 0.5f);

            float3 nTarget(kTargetTransform.position + kFollow.rotation * kFollow.offset);
                
            float4x4 nLookAt;
            nLookAt.LookAt(nTarget, kTargetTransform.position, float3(0.0f, 1.0f, 0.0f));

            auto &kCurrentTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, L"transform");
            kCurrentTransform.position = nTarget;
            kCurrentTransform.rotation = nLookAt;
        }
    }, true);
}
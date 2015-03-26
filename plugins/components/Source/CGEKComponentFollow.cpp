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
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemFollow::~CGEKComponentSystemFollow(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
}

STDMETHODIMP CGEKComponentSystemFollow::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    return hRetVal;
};

STDMETHODIMP_(void) CGEKComponentSystemFollow::OnUpdateEnd(float nGameTime, float nFrameTime)
{
    m_pEngine->GetSceneManager()->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(follow) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        auto &kFollow = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(follow)>(nEntityID, GET_COMPONENT_ID(follow));

        GEKENTITYID nTargetID = GEKINVALIDENTITYID;
        if (SUCCEEDED(m_pEngine->GetSceneManager()->GetNamedEntity(kFollow.target, &nTargetID)))
        {
            auto &kTargetTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nTargetID, GET_COMPONENT_ID(transform));

            kFollow.rotation = kFollow.rotation.Slerp(kTargetTransform.rotation, 0.5f);

            float3 nTarget(kTargetTransform.position + kFollow.rotation * kFollow.offset);
                
            float4x4 nLookAt;
            nLookAt.LookAt(nTarget, kTargetTransform.position, float3(0.0f, 1.0f, 0.0f));

            auto &kCurrentTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            kCurrentTransform.position = nTarget;
            kCurrentTransform.rotation = nLookAt;
        }
    }, true);
}
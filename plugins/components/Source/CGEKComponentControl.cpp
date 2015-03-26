#include "CGEKComponentControl.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

REGISTER_COMPONENT(control)
    REGISTER_COMPONENT_DEFAULT_VALUE(turn, 0.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(tilt, 0.0f)
    REGISTER_COMPONENT_SERIALIZE(control)
        REGISTER_COMPONENT_SERIALIZE_VALUE(turn, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(tilt, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(control)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(turn, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(tilt, StrToFloat)
END_REGISTER_COMPONENT(control)

BEGIN_INTERFACE_LIST(CGEKComponentSystemControl)
    INTERFACE_LIST_ENTRY_COM(IGEKInputObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemControl)

CGEKComponentSystemControl::CGEKComponentSystemControl(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemControl::~CGEKComponentSystemControl(void)
{
    CGEKObservable::RemoveObserver(m_pEngine, (IGEKInputObserver *)GetUnknown());
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
}

STDMETHODIMP CGEKComponentSystemControl::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine, (IGEKInputObserver *)GetUnknown());
    }

    return hRetVal;
};

STDMETHODIMP_(void) CGEKComponentSystemControl::OnState(LPCWSTR pName, bool bState)
{
    m_pEngine->GetSceneManager()->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(control) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        if (bState)
        {
            m_aConstantActions[nEntityID][pName] = 1.0f;
        }
        else
        {
            m_aConstantActions[nEntityID][pName] = 0.0f;
        }
    }, true);
}

STDMETHODIMP_(void) CGEKComponentSystemControl::OnValue(LPCWSTR pName, float nValue)
{
    m_pEngine->GetSceneManager()->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(control) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        m_aSingleActions[nEntityID][pName] = nValue;
    }, true);
}

STDMETHODIMP_(void) CGEKComponentSystemControl::OnFree(void)
{
    m_aSingleActions.clear();
    m_aConstantActions.clear();
}

STDMETHODIMP_(void) CGEKComponentSystemControl::OnUpdateBegin(float nGameTime, float nFrameTime)
{
    std::function<void(concurrency::concurrent_unordered_map<GEKENTITYID, concurrency::concurrent_unordered_map<CStringW, float>> &)> DoActions = 
        [&](concurrency::concurrent_unordered_map<GEKENTITYID, concurrency::concurrent_unordered_map<CStringW, float>> &aActions) -> void
    {
        for (auto pEntity : aActions)
        {
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(pEntity.first, GET_COMPONENT_ID(transform));
            auto &kControl = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(control)>(pEntity.first, GET_COMPONENT_ID(control));

            kControl.turn += (pEntity.second[L"turn"] * 0.01f);
            kControl.tilt = 0.0f;//+= (pEntity.second[L"tilt"] * 0.01f);

            float4x4 nRotation(float4x4(kControl.tilt, 0.0f, 0.0f) * float4x4(0.0f, kControl.turn, 0.0f));

            float3 nForce(0.0f, 0.0f, 0.0f);
            nForce += nRotation.rz * pEntity.second[L"forward"];
            nForce -= nRotation.rz * pEntity.second[L"backward"];
            nForce += nRotation.rx * pEntity.second[L"strafe_right"];
            nForce -= nRotation.rx * pEntity.second[L"strafe_left"];
            nForce += nRotation.ry * pEntity.second[L"rise"];
            nForce -= nRotation.ry * pEntity.second[L"fall"];
            nForce *= 10.0f;

            kTransform.position += (nForce * nFrameTime);
            kTransform.rotation = nRotation;
        }
    };

    DoActions(m_aSingleActions);
    m_aSingleActions.clear();
    DoActions(m_aConstantActions);
}
#include "CGEKComponentController.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentController)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentController)

CGEKComponentController::CGEKComponentController(void)
{
}

CGEKComponentController::~CGEKComponentController(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentController::GetName(void) const
{
    return L"controller";
};

STDMETHODIMP CGEKComponentController::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentController::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentController::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentController::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
}

STDMETHODIMP_(bool) CGEKComponentController::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    return false;
}

STDMETHODIMP_(bool) CGEKComponentController::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    return false;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemController)
    INTERFACE_LIST_ENTRY_COM(IGEKInputObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemController)

CGEKComponentSystemController::CGEKComponentSystemController(void)
{
}

CGEKComponentSystemController::~CGEKComponentSystemController(void)
{
}

STDMETHODIMP CGEKComponentSystemController::Initialize(void)
{
    GetContext()->AddCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
    return GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
};

STDMETHODIMP_(void) CGEKComponentSystemController::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
}

STDMETHODIMP_(void) CGEKComponentSystemController::OnAction(LPCWSTR pName, const GEKVALUE &kValue)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
    if (pSceneManager != nullptr)
    {
        pSceneManager->ListComponentsEntities({ L"transform", L"controller" }, [&](const GEKENTITYID &nEntityID)->void
        {
            m_aActions[nEntityID][pName] = kValue.GetFloat();
        });
    }
}

STDMETHODIMP_(void) CGEKComponentSystemController::OnPreUpdate(float nGameTime, float nFrameTime)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationManager);
    if (pSceneManager != nullptr)
    {
        for (auto pEntity : m_aActions)
        {
            GEKVALUE kPosition;
            GEKVALUE kRotation;
            pSceneManager->GetProperty(pEntity.first, L"transform", L"position", kPosition);
            pSceneManager->GetProperty(pEntity.first, L"transform", L"rotation", kRotation);

            float4x4 nRotation(quaternion(0.0f, pEntity.second[L"turn"] * 0.01f, pEntity.second[L"tilt"] * 0.01f) * kRotation.GetQuaternion());

            float3 nForce(0.0f, 0.0f, 0.0f);
            nForce += nRotation.rz * pEntity.second[L"forward"];
            nForce -= nRotation.rz * pEntity.second[L"backward"];
            nForce += nRotation.rx * pEntity.second[L"strafe_right"];
            nForce -= nRotation.rx * pEntity.second[L"strafe_left"];
            nForce += nRotation.ry * pEntity.second[L"rise"];
            nForce -= nRotation.ry * pEntity.second[L"fall"];
            nForce *= 10.0f;

            float3 nPosition = (kPosition.GetFloat3() + (nForce * nFrameTime));
            pSceneManager->SetProperty(pEntity.first, L"transform", L"position", nPosition);
            pSceneManager->SetProperty(pEntity.first, L"transform", L"rotation", quaternion(nRotation));
        }
    }

    m_aActions.clear();
}
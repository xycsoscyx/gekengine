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

STDMETHODIMP_(void) CGEKComponentController::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentController::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentController::RemoveComponent(const GEKENTITYID &nEntityID)
{
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);
}

STDMETHODIMP_(bool) CGEKComponentController::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.count(nEntityID) > 0);
}

STDMETHODIMP_(void) CGEKComponentController::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"turn", (*pIterator).second.m_nTurn);
        OnProperty(L"tilt", (*pIterator).second.m_nTilt);
    }
}

STDMETHODIMP_(bool) CGEKComponentController::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"turn") == 0)
        {
            kValue = (*pIterator).second.m_nTurn;
            bReturn = true;
        }
        else if (wcscmp(pName, L"tilt") == 0)
        {
            kValue = (*pIterator).second.m_nTilt;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentController::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"turn") == 0)
        {
            (*pIterator).second.m_nTurn = kValue.GetFloat();
            bReturn = true;
        }
        else if (wcscmp(pName, L"tilt") == 0)
        {
            (*pIterator).second.m_nTilt = kValue.GetFloat();
            bReturn = true;
        }
    }

    return bReturn;
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
    return GetContext()->AddCachedObserver(CLSID_GEKPopulationSystem, (IGEKSceneObserver *)GetUnknown());
};

STDMETHODIMP_(void) CGEKComponentSystemController::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationSystem, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
}

STDMETHODIMP_(void) CGEKComponentSystemController::OnAction(LPCWSTR pName, const GEKVALUE &kValue)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
    if (pSceneManager != nullptr)
    {
        pSceneManager->ListComponentsEntities({ L"transform", L"controller" }, [&](const GEKENTITYID &nEntityID)->void
        {
            m_aActions[nEntityID][pName] = kValue.GetFloat();
        }, true);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemController::OnPreUpdate(float nGameTime, float nFrameTime)
{
    IGEKSceneManager *pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
    if (pSceneManager != nullptr)
    {
        for (auto pEntity : m_aActions)
        {
            GEKVALUE kPosition;
            GEKVALUE kRotation;
            pSceneManager->GetProperty(pEntity.first, L"transform", L"position", kPosition);
            pSceneManager->GetProperty(pEntity.first, L"transform", L"rotation", kRotation);

            GEKVALUE kTurn;
            GEKVALUE kTilt;
            pSceneManager->GetProperty(pEntity.first, L"controller", L"turn", kTurn);
            pSceneManager->GetProperty(pEntity.first, L"controller", L"tilt", kTilt);

            float nTurn = (kTurn.GetFloat() + pEntity.second[L"turn"] * 0.01f);
            float nTilt = (kTilt.GetFloat() + pEntity.second[L"tilt"] * 0.01f);
            pSceneManager->SetProperty(pEntity.first, L"controller", L"turn", nTurn);
            pSceneManager->SetProperty(pEntity.first, L"controller", L"tilt", nTilt);

            float4x4 nRotation = float4x4(nTilt, 0.0f, 0.0f) * float4x4(0.0f, nTurn, 0.0f);

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
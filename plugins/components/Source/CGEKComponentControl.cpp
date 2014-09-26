#include "CGEKComponentControl.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentControl)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentControl)

CGEKComponentControl::CGEKComponentControl(void)
{
}

CGEKComponentControl::~CGEKComponentControl(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentControl::GetName(void) const
{
    return L"control";
};

STDMETHODIMP_(void) CGEKComponentControl::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentControl::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentControl::RemoveComponent(const GEKENTITYID &nEntityID)
{
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);
}

STDMETHODIMP_(bool) CGEKComponentControl::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.count(nEntityID) > 0);
}

STDMETHODIMP_(void) CGEKComponentControl::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"turn", (*pIterator).second.m_nTurn);
        OnProperty(L"tilt", (*pIterator).second.m_nTilt);
    }
}

STDMETHODIMP_(bool) CGEKComponentControl::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
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

STDMETHODIMP_(bool) CGEKComponentControl::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
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

BEGIN_INTERFACE_LIST(CGEKComponentSystemControl)
    INTERFACE_LIST_ENTRY_COM(IGEKInputObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemControl)

CGEKComponentSystemControl::CGEKComponentSystemControl(void)
    : m_pSceneManager(nullptr)
{
}

CGEKComponentSystemControl::~CGEKComponentSystemControl(void)
{
}

STDMETHODIMP CGEKComponentSystemControl::Initialize(void)
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

STDMETHODIMP_(void) CGEKComponentSystemControl::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKEngine, (IGEKInputObserver *)GetUnknown());
    if (m_pSceneManager)
    {
        CGEKObservable::RemoveObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }
}

STDMETHODIMP_(void) CGEKComponentSystemControl::OnAction(LPCWSTR pName, const GEKVALUE &kValue)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"control" }, [&](const GEKENTITYID &nEntityID)->void
    {
        m_aActions[nEntityID][pName] = kValue.GetFloat();
    }, true);
}

STDMETHODIMP_(void) CGEKComponentSystemControl::OnPreUpdate(float nGameTime, float nFrameTime)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    for (auto pEntity : m_aActions)
    {
        GEKVALUE kPosition;
        GEKVALUE kRotation;
        m_pSceneManager->GetProperty(pEntity.first, L"transform", L"position", kPosition);
        m_pSceneManager->GetProperty(pEntity.first, L"transform", L"rotation", kRotation);

        GEKVALUE kTurn;
        GEKVALUE kTilt;
        m_pSceneManager->GetProperty(pEntity.first, L"Control", L"turn", kTurn);
        m_pSceneManager->GetProperty(pEntity.first, L"Control", L"tilt", kTilt);

        float nTurn = (kTurn.GetFloat() + pEntity.second[L"turn"] * 0.01f);
        float nTilt = (kTilt.GetFloat() + pEntity.second[L"tilt"] * 0.01f);
        m_pSceneManager->SetProperty(pEntity.first, L"Control", L"turn", nTurn);
        m_pSceneManager->SetProperty(pEntity.first, L"Control", L"tilt", nTilt);

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
        m_pSceneManager->SetProperty(pEntity.first, L"transform", L"position", nPosition);
        m_pSceneManager->SetProperty(pEntity.first, L"transform", L"rotation", quaternion(nRotation));
    }

    m_aActions.clear();
}
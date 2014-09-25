#include "CGEKComponentFollow.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentFollow)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentFollow)

CGEKComponentFollow::CGEKComponentFollow(void)
{
}

CGEKComponentFollow::~CGEKComponentFollow(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentFollow::GetName(void) const
{
    return L"follow";
};

STDMETHODIMP_(void) CGEKComponentFollow::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentFollow::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentFollow::RemoveComponent(const GEKENTITYID &nEntityID)
{
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);
}

STDMETHODIMP_(bool) CGEKComponentFollow::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.count(nEntityID) > 0);
}

STDMETHODIMP_(void) CGEKComponentFollow::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"target", (*pIterator).second.m_strTarget.GetString());
        OnProperty(L"offset", (*pIterator).second.m_nOffset);
    }
}

STDMETHODIMP_(bool) CGEKComponentFollow::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"target") == 0)
        {
            kValue = (*pIterator).second.m_strTarget.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"offset") == 0)
        {
            kValue = (*pIterator).second.m_nOffset;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentFollow::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"target") == 0)
        {
            (*pIterator).second.m_strTarget = kValue.GetRawString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"offset") == 0)
        {
            (*pIterator).second.m_nOffset = kValue.GetFloat3();
            bReturn = true;
        }
    }

    return bReturn;
}

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

STDMETHODIMP_(void) CGEKComponentSystemFollow::OnPostUpdate(float nGameTime, float nFrameTime)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"follow" }, [&](const GEKENTITYID &nEntityID)->void
    {
    }, true);
}
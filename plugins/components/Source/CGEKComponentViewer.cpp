#include "CGEKComponentViewer.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentViewer)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentViewer)

CGEKComponentViewer::CGEKComponentViewer(void)
{
}

CGEKComponentViewer::~CGEKComponentViewer(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentViewer::GetName(void) const
{
    return L"viewer";
};

STDMETHODIMP CGEKComponentViewer::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentViewer::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentViewer::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentViewer::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"fieldofview", (*pIterator).second.m_nFieldOfView);
        OnProperty(L"minviewdistance", (*pIterator).second.m_nMinViewDistance);
        OnProperty(L"maxviewdistance", (*pIterator).second.m_nMaxViewDistance);
    }
}

STDMETHODIMP_(bool) CGEKComponentViewer::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"fieldofview") == 0)
        {
            kValue = (*pIterator).second.m_nFieldOfView;
            bReturn = true;
        }
        else if (wcscmp(pName, L"minviewdistance") == 0)
        {
            kValue = (*pIterator).second.m_nMinViewDistance;
            bReturn = true;
        }
        else if (wcscmp(pName, L"maxviewdistance") == 0)
        {
            kValue = (*pIterator).second.m_nMaxViewDistance;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentViewer::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"fieldofview") == 0)
        {
            (*pIterator).second.m_nFieldOfView = kValue.GetFloat();
            bReturn = true;
        }
        else if (wcscmp(pName, L"minviewdistance") == 0)
        {
            (*pIterator).second.m_nMinViewDistance = kValue.GetFloat();
            bReturn = true;
        }
        else if (wcscmp(pName, L"maxviewdistance") == 0)
        {
            (*pIterator).second.m_nMaxViewDistance = kValue.GetFloat();
            bReturn = true;
        }
    }

    return bReturn;
}

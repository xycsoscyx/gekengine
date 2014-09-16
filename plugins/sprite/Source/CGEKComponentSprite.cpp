#include "CGEKComponentSprite.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentSprite)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSprite)

CGEKComponentSprite::CGEKComponentSprite(void)
{
}

CGEKComponentSprite::~CGEKComponentSprite(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentSprite::GetName(void) const
{
    return L"sprite";
};

STDMETHODIMP_(void) CGEKComponentSprite::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentSprite::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentSprite::RemoveComponent(const GEKENTITYID &nEntityID)
{
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);
}

STDMETHODIMP_(bool) CGEKComponentSprite::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.count(nEntityID) > 0);
}

STDMETHODIMP_(void) CGEKComponentSprite::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"source", (*pIterator).second.m_strSource.GetString());
        OnProperty(L"size", (*pIterator).second.m_nSize);
        OnProperty(L"color", (*pIterator).second.m_nColor);
    }
}

STDMETHODIMP_(bool) CGEKComponentSprite::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"source") == 0)
        {
            kValue = (*pIterator).second.m_strSource.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"size") == 0)
        {
            kValue = (*pIterator).second.m_nSize;
            bReturn = true;
        }
        else if (wcscmp(pName, L"color") == 0)
        {
            kValue = (*pIterator).second.m_nColor;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentSprite::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"source") == 0)
        {
            (*pIterator).second.m_strSource = kValue.GetRawString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"size") == 0)
        {
            (*pIterator).second.m_nSize = kValue.GetFloat();
            bReturn = true;
        }
        else if (wcscmp(pName, L"color") == 0)
        {
            (*pIterator).second.m_nColor = kValue.GetFloat4();
            bReturn = true;
        }
    }

    return bReturn;
}

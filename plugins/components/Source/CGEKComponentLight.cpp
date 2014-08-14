#include "CGEKComponentLight.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentLight)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentLight)

CGEKComponentLight::CGEKComponentLight(void)
{
}

CGEKComponentLight::~CGEKComponentLight(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentLight::GetName(void) const
{
    return L"light";
};

STDMETHODIMP_(void) CGEKComponentLight::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentLight::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentLight::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentLight::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentLight::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"color", (*pIterator).second.m_nColor);
        OnProperty(L"range", (*pIterator).second.m_nRange);
    }
}

STDMETHODIMP_(bool) CGEKComponentLight::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"color") == 0)
        {
            kValue = (*pIterator).second.m_nColor;
            bReturn = true;
        }
        else if (wcscmp(pName, L"range") == 0)
        {
            kValue = (*pIterator).second.m_nRange;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentLight::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"color") == 0)
        {
            (*pIterator).second.m_nColor = kValue.GetFloat3();
            bReturn = true;
        }
        else if (wcscmp(pName, L"range") == 0)
        {
            (*pIterator).second.m_nRange = kValue.GetFloat();
            bReturn = true;
        }
    }

    return bReturn;
}

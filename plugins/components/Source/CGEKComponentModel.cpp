#include "CGEKComponentModel.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentModel)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentModel)

CGEKComponentModel::CGEKComponentModel(void)
{
}

CGEKComponentModel::~CGEKComponentModel(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentModel::GetName(void) const
{
    return L"model";
};

STDMETHODIMP_(void) CGEKComponentModel::Clear(void)
{
    m_aData.clear();
}

STDMETHODIMP CGEKComponentModel::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentModel::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentModel::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentModel::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"source", (*pIterator).second.m_strSource.GetString());
        OnProperty(L"params", (*pIterator).second.m_strParams.GetString());
        OnProperty(L"scale", (*pIterator).second.m_nScale);
    }
}

STDMETHODIMP_(bool) CGEKComponentModel::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
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
        else if (wcscmp(pName, L"params") == 0)
        {
            kValue = (*pIterator).second.m_strParams.GetString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"scale") == 0)
        {
            kValue = (*pIterator).second.m_nScale;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentModel::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
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
        else if (wcscmp(pName, L"params") == 0)
        {
            (*pIterator).second.m_strParams = kValue.GetRawString();
            bReturn = true;
        }
        else if (wcscmp(pName, L"scale") == 0)
        {
            (*pIterator).second.m_nScale = kValue.GetFloat3();
            bReturn = true;
        }
    }

    return bReturn;
}

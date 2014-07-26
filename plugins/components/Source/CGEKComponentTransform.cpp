#include "CGEKComponentTransform.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentTransform)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentTransform)

CGEKComponentTransform::CGEKComponentTransform(void)
{
}

CGEKComponentTransform::~CGEKComponentTransform(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentTransform::GetName(void) const
{
    return L"transform";
};

STDMETHODIMP CGEKComponentTransform::AddComponent(const GEKENTITYID &nEntityID)
{
    m_aData[nEntityID] = DATA();
    return S_OK;
}

STDMETHODIMP CGEKComponentTransform::RemoveComponent(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        m_aData.unsafe_erase(pIterator);
    }

    return S_OK;
}

STDMETHODIMP_(bool) CGEKComponentTransform::HasComponent(const GEKENTITYID &nEntityID) const
{
    return (m_aData.find(nEntityID) != m_aData.end());
}

STDMETHODIMP_(void) CGEKComponentTransform::ListProperties(const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        OnProperty(L"position", (*pIterator).second.m_nPosition);
        OnProperty(L"rotation", (*pIterator).second.m_nRotation);
    }
}

STDMETHODIMP_(bool) CGEKComponentTransform::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"position") == 0)
        {
            kValue = (*pIterator).second.m_nPosition;
            bReturn = true;
        }
        else if (wcscmp(pName, L"rotation") == 0)
        {
            kValue = (*pIterator).second.m_nRotation;
            bReturn = true;
        }
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKComponentTransform::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aData.find(nEntityID);
    if (pIterator != m_aData.end())
    {
        if (wcscmp(pName, L"position") == 0)
        {
            (*pIterator).second.m_nPosition = kValue.GetFloat3();
            bReturn = true;
        }
        else if (wcscmp(pName, L"rotation") == 0)
        {
            (*pIterator).second.m_nRotation = kValue.GetQuaternion();
            bReturn = true;
        }
    }

    return bReturn;
}

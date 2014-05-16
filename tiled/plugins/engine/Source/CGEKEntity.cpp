#include "CGEKEntity.h"
#include <amp.h>
#include <functional>

BEGIN_INTERFACE_LIST(CGEKEntity)
    INTERFACE_LIST_ENTRY_COM(IGEKEntity)
END_INTERFACE_LIST

CGEKEntity::CGEKEntity(LPCWSTR pFlags)
    : m_strFlags(pFlags)
{
}

CGEKEntity::~CGEKEntity(void)
{
}

HRESULT CGEKEntity::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    for (auto &kPair : m_aComponents)
    {
        hRetVal = kPair.second->OnEntityCreated();
        if (FAILED(hRetVal))
        {
            break;
        }
    }

    return hRetVal;
}

void CGEKEntity::OnEntityDestroyed(void)
{
    for (auto &kPair : m_aComponents)
    {
        kPair.second->OnEntityDestroyed();
    }
}

HRESULT CGEKEntity::AddComponent(IGEKComponent *pComponent)
{
    REQUIRE_RETURN(pComponent, E_INVALIDARG);
    m_aComponents[pComponent->GetType()] = pComponent;
    return S_OK;
}

STDMETHODIMP_(void) CGEKEntity::ListComponents(std::function<void(IGEKComponent *)> OnComponent)
{
    for (auto &kPair : m_aComponents)
    {
        OnComponent(kPair.second);
    }
}

LPCWSTR CGEKEntity::GetFlags(void)
{
    return m_strFlags;
}

STDMETHODIMP_(IGEKComponent *) CGEKEntity::GetComponent(LPCWSTR pName)
{
    auto pIterator = m_aComponents.find(pName);
    if (pIterator != m_aComponents.end())
    {
        return ((*pIterator).second);
    }
    else
    {
        return nullptr;
    }
}

STDMETHODIMP_(void) CGEKEntity::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    for (auto &kPair : m_aComponents)
    {
        kPair.second->OnEvent(pAction, kParamA, kParamB);
    }
}

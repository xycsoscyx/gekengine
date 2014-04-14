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
    std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<GEKHASH, CComPtr<IGEKComponent>>::value_type &kPair) -> bool
    {
        hRetVal = kPair.second->OnEntityCreated();
        return FAILED(hRetVal);
    });

    return hRetVal;
}

HRESULT CGEKEntity::OnEntityDestroyed(void)
{
    for (auto &kPair : m_aComponents)
    {
        kPair.second->OnEntityDestroyed();
    }

    return S_OK;
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
    GEKHASH nHash(pName);
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [nHash](std::map<GEKHASH, CComPtr<IGEKComponent>>::value_type &kPair) -> bool
    {
        return (kPair.first == nHash);
    } );

    if (pIterator == m_aComponents.end())
    {
        return nullptr;
    }
    else
    {
        return ((*pIterator).second);
    }
}

STDMETHODIMP CGEKEntity::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    HRESULT hRetVal = S_OK;
    std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<GEKHASH, CComPtr<IGEKComponent>>::value_type &kPair) -> bool
    {
        hRetVal = kPair.second->OnEvent(pAction, kParamA, kParamB);
        return FAILED(hRetVal);
    });

    return hRetVal;
}

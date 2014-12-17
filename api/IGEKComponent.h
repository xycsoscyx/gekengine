#pragma once

#include "GEKUtility.h"
#include "IGEKSceneManager.h"
#include <concurrent_unordered_map.h>
#include <unordered_map>

#pragma warning(disable:4503)

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const PURE;
    STDMETHOD_(void, Clear)                     (THIS) PURE;

    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const PURE;
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID) PURE;

    STDMETHOD(Serialize)                        (THIS_ const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams) PURE;
    STDMETHOD(DeSerialize)                      (THIS_ const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams) PURE;

    template <typename CLASS>
    CLASS &GetComponent(const GEKENTITYID &nEntityID)
    {
        return *(CLASS *)GetComponent(nEntityID);
    }
};

#define DECLARE_COMPONENT(NAME)                                                             \
class CGEKComponent##NAME##                                                                 \
    : public CGEKUnknown                                                                    \
    , public IGEKComponent                                                                  \
{                                                                                           \
public:                                                                                     \
    DECLARE_UNKNOWN(CGEKComponent##NAME##);                                                 \
    CGEKComponent##NAME##(void);                                                            \
    ~CGEKComponent##NAME##(void);                                                           \
    STDMETHOD_(LPVOID, GetComponent)(THIS_ const GEKENTITYID &nEntityID);                   \
    STDMETHOD_(LPCWSTR, GetName)    (THIS) const;                                           \
    STDMETHOD_(void, Clear)         (THIS);                                                 \
    STDMETHOD(AddComponent)         (THIS_ const GEKENTITYID &nEntityID);                   \
    STDMETHOD(RemoveComponent)      (THIS_ const GEKENTITYID &nEntityID);                   \
    STDMETHOD_(bool, HasComponent)  (THIS_ const GEKENTITYID &nEntityID) const;             \
    STDMETHOD(Serialize)            (THIS_ const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams);      \
    STDMETHOD(DeSerialize)          (THIS_ const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams);\
                                                                                            \
public:                                                                                     \
    struct DATA                                                                             \
    {

#define DECLARE_COMPONENT_DATA(TYPE, NAME)                                                  TYPE NAME;

#define END_DECLARE_COMPONENT(NAME)                                                         \
    };                                                                                      \
                                                                                            \
private:                                                                                    \
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;                       \
};

#define GET_COMPONENT_DATA(NAME)                                                            CGEKComponent##NAME##::DATA

#define REGISTER_COMPONENT(NAME)                                                            \
CGEKComponent##NAME##::CGEKComponent##NAME##(void)                                          \
{                                                                                           \
}                                                                                           \
                                                                                            \
CGEKComponent##NAME##::~CGEKComponent##NAME##(void)                                         \
{                                                                                           \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(LPCWSTR) CGEKComponent##NAME##::GetName(void) const                           \
{                                                                                           \
    return L#NAME;                                                                          \
};                                                                                          \
                                                                                            \
STDMETHODIMP_(void) CGEKComponent##NAME##::Clear(void)                                      \
{                                                                                           \
    m_aData.clear();                                                                        \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::AddComponent(const GEKENTITYID &nEntityID)              \
{                                                                                           \
    m_aData[nEntityID] = DATA();                                                            \
    return S_OK;                                                                            \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::RemoveComponent(const GEKENTITYID &nEntityID)           \
{                                                                                           \
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);                           \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(bool) CGEKComponent##NAME##::HasComponent(const GEKENTITYID &nEntityID) const \
{                                                                                           \
    return (m_aData.count(nEntityID) > 0);                                                  \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(LPVOID) CGEKComponent##NAME##::GetComponent(const GEKENTITYID &nEntityID)     \
{                                                                                           \
    auto pIterator = m_aData.find(nEntityID);                                               \
    if (pIterator != m_aData.end())                                                         \
    {                                                                                       \
        return LPVOID(&(*pIterator).second);                                                \
    }                                                                                       \
                                                                                            \
    return nullptr;                                                                         \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::Serialize(const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams)  \
{                                                                                           \
    HRESULT hRetVal = E_FAIL;                                                               \
    auto pIterator = m_aData.find(nEntityID);                                               \
    if (pIterator != m_aData.end())                                                         \
    {

#define REGISTER_SERIALIZE(VALUE, SERIALIZE)                                                aParams[L#VALUE] = SERIALIZE((*pIterator).second.VALUE);

#define REGISTER_SEPARATOR(NAME)                                                            \
        hRetVal = S_OK;                                                                     \
    }                                                                                       \
                                                                                            \
    return hRetVal;                                                                         \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::DeSerialize(const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams)  \
{                                                                                           \
    HRESULT hRetVal = E_FAIL;                                                               \
    auto pIterator = m_aData.find(nEntityID);                                               \
    if (pIterator != m_aData.end())                                                         \
    {

#define REGISTER_DESERIALIZE(VALUE, DESERIALIZE)                                            if(true) { auto pValue = aParams.find(L#VALUE); if(pValue != aParams.end()) { (*pIterator).second.VALUE = DESERIALIZE((*pValue).second); }; };

#define END_REGISTER_COMPONENT(NAME)                                                        \
        hRetVal = S_OK;                                                                     \
    }                                                                                       \
                                                                                            \
    return hRetVal;                                                                         \
}                                                                                           \
                                                                                            \
BEGIN_INTERFACE_LIST(CGEKComponent##NAME##)                                                 \
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)                                                 \
END_INTERFACE_LIST_UNKNOWN                                                                  \
                                                                                            \
REGISTER_CLASS(CGEKComponent##NAME##)

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
};

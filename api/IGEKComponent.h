#pragma once

#include "GEKUtility.h"
#include "GEKVALUE.h"
#include "IGEKSceneManager.h"

#pragma warning(disable:4503)

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
private:
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID) PURE;

public:
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const PURE;
    STDMETHOD_(void, Clear)                     (THIS) PURE;

    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const PURE;

    template <typename CLASS>
    CLASS &GetComponent(const GEKENTITYID &nEntityID)
    {
        return *(CLASS *)GetComponent(nEntityID);
    }
};

#define DECLARE_COMPONENT(NAME)                                         \
class CGEKComponent##NAME                                               \
    : public CGEKUnknown                                                \
    , public IGEKComponent                                              \
{                                                                       \
public:                                                                 \
    DECLARE_UNKNOWN(CGEKComponent##NAME);                               \
    CGEKComponent##NAME(void);                                          \
    ~CGEKComponent##NAME(void);                                         \
    STDMETHOD_(LPVOID, GetComponent)(THIS_ const GEKENTITYID &nEntityID);       \
    STDMETHOD_(LPCWSTR, GetName)    (THIS) const;                               \
    STDMETHOD_(void, Clear)         (THIS);                                     \
    STDMETHOD(AddComponent)         (THIS_ const GEKENTITYID &nEntityID);       \
    STDMETHOD(RemoveComponent)      (THIS_ const GEKENTITYID &nEntityID);       \
    STDMETHOD_(bool, HasComponent)  (THIS_ const GEKENTITYID &nEntityID) const; \
                                                                        \
public:                                                                 \
    struct DATA                                                         \
    {

#define DECLARE_COMPONENT_DATA(TYPE, NAME)                              \
        TYPE NAME;

#define END_COMPONENT(NAME)                                             \
    };                                                                  \
                                                                        \
private:                                                                \
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;   \
};

#define COMPONENT_DATA(NAME)                                            CGEKComponent##NAME::DATA

#define REGISTER_COMPONENT(NAME)                                                        \
CGEKComponent##NAME::CGEKComponent##NAME(void)                                          \
{                                                                                       \
}                                                                                       \
                                                                                        \
CGEKComponent##NAME::~CGEKComponent##NAME(void)                                         \
{                                                                                       \
}                                                                                       \
                                                                                        \
STDMETHODIMP_(LPVOID) CGEKComponent##NAME::GetComponent(const GEKENTITYID &nEntityID)   \
{                                                                                       \
    auto pIterator = m_aData.find(nEntityID);                                           \
    if (pIterator != m_aData.end())                                                     \
    {                                                                                   \
        return LPVOID(&(*pIterator).second);                                            \
    }                                                                                   \
                                                                                        \
    return nullptr;                                                                     \
}                                                                                       \
                                                                                        \
STDMETHODIMP_(LPCWSTR) CGEKComponent##NAME::GetName(void) const                         \
{                                                                                       \
    return L#NAME;                                                                      \
};                                                                                      \
                                                                                        \
STDMETHODIMP_(void) CGEKComponent##NAME::Clear(void)                                    \
{                                                                                       \
    m_aData.clear();                                                                    \
}                                                                                       \
                                                                                        \
STDMETHODIMP CGEKComponent##NAME::AddComponent(const GEKENTITYID &nEntityID)            \
{                                                                                       \
    m_aData[nEntityID] = DATA();                                                        \
    return S_OK;                                                                        \
}                                                                                       \
                                                                                        \
STDMETHODIMP CGEKComponent##NAME::RemoveComponent(const GEKENTITYID &nEntityID)         \
{                                                                                       \
    return (m_aData.unsafe_erase(nEntityID) > 0 ? S_OK : E_FAIL);                       \
}                                                                                       \
                                                                                        \
STDMETHODIMP_(bool) CGEKComponent##NAME::HasComponent(const GEKENTITYID &nEntityID) const   \
{                                                                                       \
    return (m_aData.count(nEntityID) > 0);                                              \
}                                                                                       \
                                                                                        \
BEGIN_INTERFACE_LIST(CGEKComponent##NAME)                                               \
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)                                             \
END_INTERFACE_LIST_UNKNOWN                                                              \
                                                                                        \
REGISTER_CLASS(CGEKComponent##NAME)

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
};

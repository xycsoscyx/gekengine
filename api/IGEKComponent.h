#pragma once

#include "GEKUtility.h"
#include "IGEKSceneManager.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
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

#define DECLARE_COMPONENT_VALUE(TYPE, VALUE)                                                TYPE VALUE;

#define END_DECLARE_COMPONENT(NAME)                                                         \
    };                                                                                      \
                                                                                            \
private:                                                                                    \
    concurrency::concurrent_queue<UINT32> m_aEmpty;                                         \
    concurrency::concurrent_unordered_map<GEKENTITYID, UINT32> m_aIndices;                  \
    concurrency::concurrent_vector<DATA> m_aData;                                           \
};

#define GET_COMPONENT_DATA(NAME)                                                            CGEKComponent##NAME##::DATA

#define REGISTER_COMPONENT(NAME)                                                            \
BEGIN_INTERFACE_LIST(CGEKComponent##NAME##)                                                 \
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)                                                 \
END_INTERFACE_LIST_UNKNOWN                                                                  \
                                                                                            \
REGISTER_CLASS(CGEKComponent##NAME##)                                                       \
                                                                                            \
CGEKComponent##NAME##::CGEKComponent##NAME##(void)                                          \
{

#define REGISTER_COMPONENT_DEFAULT_VALUE(VALUE, DEFAULT)                                    VALUE = DEFAULT;

#define REGISTER_COMPONENT_SERIALIZE(NAME)                                                  \
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
    m_aEmpty.clear();                                                                       \
    m_aIndices.clear();                                                                     \
    m_aData.clear();                                                                        \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::AddComponent(const GEKENTITYID &nEntityID)              \
{                                                                                           \
    UINT32 nDataID = 0;                                                                     \
    if (m_aEmpty.try_pop(nDataID))                                                          \
    {                                                                                       \
        m_aIndices[nEntityID] = nDataID;                                                    \
        m_aData[nDataID] = DATA();                                                          \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        m_aIndices[nEntityID] = m_aData.size();                                             \
        m_aData.push_back(DATA());                                                          \
    }                                                                                       \
                                                                                            \
    return S_OK;                                                                            \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::RemoveComponent(const GEKENTITYID &nEntityID)           \
{                                                                                           \
    HRESULT hRetVal = E_FAIL;                                                               \
    auto pIterator = m_aIndices.find(nEntityID);                                            \
    if (pIterator != m_aIndices.end())                                                      \
    {                                                                                       \
        m_aEmpty.push((*pIterator).second);                                                 \
        m_aIndices.unsafe_erase(pIterator);                                                 \
        hRetVal = S_OK;                                                                     \
    }                                                                                       \
                                                                                            \
    return hRetVal;                                                                         \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(bool) CGEKComponent##NAME##::HasComponent(const GEKENTITYID &nEntityID) const \
{                                                                                           \
    return (m_aIndices.count(nEntityID) > 0);                                               \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(LPVOID) CGEKComponent##NAME##::GetComponent(const GEKENTITYID &nEntityID)     \
{                                                                                           \
    auto pIterator = m_aIndices.find(nEntityID);                                            \
    if (pIterator != m_aIndices.end())                                                      \
    {                                                                                       \
        return LPVOID(&m_aData[(*pIterator).second]);                                       \
    }                                                                                       \
                                                                                            \
    return nullptr;                                                                         \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::Serialize(const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams)  \
{                                                                                           \
    HRESULT hRetVal = E_FAIL;                                                               \
    auto pIterator = m_aIndices.find(nEntityID);                                            \
    if (pIterator != m_aIndices.end())                                                      \
    {                                                                                       \
        DATA &kData = m_aData[(*pIterator).second];

#define REGISTER_COMPONENT_SERIALIZE_VALUE(VALUE, SERIALIZE)                                aParams[L#VALUE] = SERIALIZE(kData.VALUE);

#define REGISTER_COMPONENT_DESERIALIZE(NAME)                                                \
        hRetVal = S_OK;                                                                     \
    }                                                                                       \
                                                                                            \
    return hRetVal;                                                                         \
}                                                                                           \
                                                                                            \
STDMETHODIMP CGEKComponent##NAME##::DeSerialize(const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams)  \
{                                                                                           \
    HRESULT hRetVal = E_FAIL;                                                               \
    auto pIterator = m_aIndices.find(nEntityID);                                            \
    if (pIterator != m_aIndices.end())                                                      \
    {                                                                                       \
        DATA &kData = m_aData[(*pIterator).second];

#define REGISTER_COMPONENT_DESERIALIZE_VALUE(VALUE, DESERIALIZE)                            if(true) { auto pValue = aParams.find(L#VALUE); if(pValue != aParams.end()) { kData.VALUE = DESERIALIZE((*pValue).second); }; };

#define END_REGISTER_COMPONENT(NAME)                                                        \
        hRetVal = S_OK;                                                                     \
    }                                                                                       \
                                                                                            \
    return hRetVal;                                                                         \
}

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
};

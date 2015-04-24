#pragma once

#include "GEKUtility.h"
#include "IGEKSceneManager.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <unordered_map>
#include <set>

#pragma warning(disable:4503)

DECLARE_INTERFACE(IGEKEngineCore);

template <typename DATA>
class TGEKComponent
{
private:
    std::vector<DATA> m_aData;
    std::unordered_map<GEKENTITYID, UINT32> m_aIndices;
    UINT32 m_nEmptyIndex;

public:
    TGEKComponent(void)
        : m_nEmptyIndex(0)
    {
    }

    void CreateComponent(const GEKENTITYID &nEntityID)
    {
        if (m_nEmptyIndex < m_aData.size())
        {
            m_aIndices[nEntityID] = m_nEmptyIndex;
            m_aData[m_nEmptyIndex] = DATA();
            m_nEmptyIndex++;
        }
        else
        {
            m_aIndices[nEntityID] = m_nEmptyIndex;
            m_aData.push_back(DATA());
            m_nEmptyIndex = m_aData.size();
        }
    }

    void DestroyComponent(const GEKENTITYID &nEntityID)
    {
        if (m_aIndices.size() == 1)
        {
            m_aIndices.clear();
            m_nEmptyIndex = 0;
        }
        else
        {
            auto pDestroyIterator = m_aIndices.find(nEntityID);
            if (pDestroyIterator != m_aIndices.end())
            {
                m_nEmptyIndex--;
                auto pMovingIterator = std::find_if(m_aIndices.begin(), m_aIndices.end(), [&](std::pair<const GEKENTITYID, UINT32> &kPair) -> bool
                {
                    return (kPair.second == m_nEmptyIndex);
                });

                if (pMovingIterator != m_aIndices.end())
                {
                    m_aData[(*pDestroyIterator).second] = std::move(m_aData.back());
                    m_aIndices[(*pMovingIterator).first] = (*pDestroyIterator).second;
                }

                m_aIndices.erase(pDestroyIterator);
            }
        }
    }

    bool HasComponent(const GEKENTITYID &nEntityID) const
    {
        return (m_aIndices.count(nEntityID) > 0);
    }

    DATA &GetComponent(const GEKENTITYID &nEntityID)
    {
        auto pIterator = m_aIndices.find(nEntityID);
        if (pIterator == m_aIndices.end())
        {
            throw std::exception("Unable to locate entity in component index map");
        }

        return m_aData[(*pIterator).second];
    }

    const DATA &GetComponent(const GEKENTITYID &nEntityID) const
    {
        auto pIterator = m_aIndices.find(nEntityID);
        if (pIterator == m_aIndices.end())
        {
            throw std::exception("Unable to locate entity in component index map");
        }

        return m_aData[(*pIterator).second];
    }
};

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
    STDMETHOD_(void, GetIntersectingSet)        (THIS_ std::set<GEKENTITYID> &aSet) PURE;

    STDMETHOD_(LPCWSTR, GetName)                (THIS) const PURE;
    STDMETHOD_(GEKCOMPONENTID, GetID)           (THIS) const PURE;
    STDMETHOD_(void, Clear)                     (THIS) PURE;

    STDMETHOD_(void, CreateComponent)           (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(void, DestroyComponent)          (THIS_ const GEKENTITYID &nEntityID) PURE;
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

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine) PURE;
};

#define DECLARE_COMPONENT(NAME, ID)                                                         \
class CGEKComponent##NAME##                                                                 \
    : public CGEKUnknown                                                                    \
    , public IGEKComponent                                                                  \
{                                                                                           \
public:                                                                                     \
    DECLARE_UNKNOWN(CGEKComponent##NAME##);                                                 \
    CGEKComponent##NAME##(void);                                                            \
    ~CGEKComponent##NAME##(void);                                                           \
                                                                                            \
    STDMETHOD_(void, GetIntersectingSet)(THIS_ std::set<GEKENTITYID> &aSet);                \
    STDMETHOD_(LPCWSTR, GetName)        (THIS) const;                                       \
    STDMETHOD_(GEKCOMPONENTID, GetID)   (THIS) const;                                       \
    STDMETHOD_(void, Clear)             (THIS);                                             \
    STDMETHOD_(void, CreateComponent)   (THIS_ const GEKENTITYID &nEntityID);               \
    STDMETHOD_(void, DestroyComponent)  (THIS_ const GEKENTITYID &nEntityID);               \
    STDMETHOD_(bool, HasComponent)      (THIS_ const GEKENTITYID &nEntityID) const;         \
    STDMETHOD_(LPVOID, GetComponent)    (THIS_ const GEKENTITYID &nEntityID);               \
    STDMETHOD(Serialize)                (THIS_ const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams);          \
    STDMETHOD(DeSerialize)              (THIS_ const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams);    \
                                                                                            \
public:                                                                                     \
    static const GEKCOMPONENTID gs_nComponentID = ID;                                       \
                                                                                            \
public:                                                                                     \
    struct BASE { };                                                                        \
                                                                                            \
    struct DATA : public BASE                                                               \
    {

#define DECLARE_COMPONENT_VALUE(TYPE, VALUE)                                                TYPE VALUE;

#define END_DECLARE_COMPONENT(NAME)                                                         \
        DATA(void *pData);                                                                  \
        DATA(void);                                                                         \
    };                                                                                      \
                                                                                            \
private:                                                                                    \
    concurrency::concurrent_queue<UINT32> m_aEmpty;                                         \
    concurrency::concurrent_unordered_map<GEKENTITYID, UINT32> m_aIndices;                  \
    concurrency::concurrent_vector<DATA> m_aData;                                           \
};

#define GET_COMPONENT_ID(NAME)                                                              CGEKComponent##NAME##::gs_nComponentID
#define GET_COMPONENT_DATA(NAME)                                                            CGEKComponent##NAME##::DATA

#define REGISTER_COMPONENT(NAME)                                                            \
BEGIN_INTERFACE_LIST(CGEKComponent##NAME##)                                                 \
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)                                                 \
END_INTERFACE_LIST_UNKNOWN                                                                  \
                                                                                            \
REGISTER_CLASS(CGEKComponent##NAME##)                                                       \
                                                                                            \
CGEKComponent##NAME##::DATA::DATA(void)                                                     \
    : BASE()

#define REGISTER_COMPONENT_DEFAULT_VALUE(VALUE, DEFAULT)                                    , VALUE(DEFAULT)

#define REGISTER_COMPONENT_SERIALIZE(NAME)                                                  \
{                                                                                           \
}                                                                                           \
                                                                                            \
CGEKComponent##NAME##::CGEKComponent##NAME##(void)                                          \
{                                                                                           \
}                                                                                           \
                                                                                            \
CGEKComponent##NAME##::~CGEKComponent##NAME##(void)                                         \
{                                                                                           \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(void) CGEKComponent##NAME##::GetIntersectingSet(std::set<GEKENTITYID> &aSet)  \
{                                                                                           \
    if (aSet.empty())                                                                       \
    {                                                                                       \
        for (auto &kPair : m_aIndices)                                                      \
        {                                                                                   \
            aSet.insert(kPair.first);                                                       \
        }                                                                                   \
    }                                                                                       \
    else                                                                                    \
    {                                                                                       \
        std::set<GEKENTITYID> aIntersection;                                                \
        for (auto &kPair : m_aIndices)                                                      \
        {                                                                                   \
            if (aSet.count(kPair.first) > 0)                                                \
            {                                                                               \
                aIntersection.insert(kPair.first);                                          \
            }                                                                               \
        }                                                                                   \
                                                                                            \
        aSet = std::move(aIntersection);                                                    \
    }                                                                                       \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(LPCWSTR) CGEKComponent##NAME##::GetName(void) const                           \
{                                                                                           \
    return L#NAME;                                                                          \
};                                                                                          \
                                                                                            \
STDMETHODIMP_(GEKCOMPONENTID) CGEKComponent##NAME##::GetID(void) const                      \
{                                                                                           \
    return gs_nComponentID;                                                                 \
};                                                                                          \
                                                                                            \
STDMETHODIMP_(void) CGEKComponent##NAME##::Clear(void)                                      \
{                                                                                           \
    m_aEmpty.clear();                                                                       \
    m_aIndices.clear();                                                                     \
    m_aData.clear();                                                                        \
}                                                                                           \
                                                                                            \
STDMETHODIMP_(void) CGEKComponent##NAME##::CreateComponent(const GEKENTITYID &nEntityID)    \
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
}                                                                                           \
                                                                                            \
STDMETHODIMP_(void) CGEKComponent##NAME##::DestroyComponent(const GEKENTITYID &nEntityID)   \
{                                                                                           \
    auto pIterator = m_aIndices.find(nEntityID);                                            \
    if (pIterator != m_aIndices.end())                                                      \
    {                                                                                       \
        m_aEmpty.push((*pIterator).second);                                                 \
        m_aIndices.unsafe_erase(pIterator);                                                 \
    }                                                                                       \
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
STDMETHODIMP CGEKComponent##NAME##::Serialize(const GEKENTITYID &nEntityID, std::unordered_map<CStringW, CStringW> &aParams)            \
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
STDMETHODIMP CGEKComponent##NAME##::DeSerialize(const GEKENTITYID &nEntityID, const std::unordered_map<CStringW, CStringW> &aParams)    \
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

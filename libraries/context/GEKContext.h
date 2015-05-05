#pragma once

// Interfaces
#include "Public\IGEKObservable.h"
#include "Public\IGEKUnknown.h"
#include "Public\IGEKContext.h"
#include <atlpath.h>
#include <algorithm>
#include <map>
#include <concurrent_unordered_set.h>

class CGEKUnknown : public IGEKUnknown
{
private:
    ULONG m_nRefCount;
    IGEKContext *m_pContext;

public:
    CGEKUnknown(IGEKContext *pContext = nullptr)
        : m_nRefCount(0)
        , m_pContext(pContext)
    {
    }

    virtual ~CGEKUnknown(void)
    {
    }

    STDMETHOD_(ULONG, AddRef)                   (THIS)
    {
        return InterlockedIncrement(&m_nRefCount);
    }

    STDMETHOD_(ULONG, Release)                  (THIS)
    {
        LONG nRefCount = InterlockedDecrement(&m_nRefCount);
        if (nRefCount == 0)
        {
            delete this;
        }

        return nRefCount;
    }

    STDMETHOD(QueryInterface)                   (THIS_ REFIID nIID, LPVOID FAR *ppObject)
    {
        REQUIRE_RETURN(ppObject, E_INVALIDARG);

        HRESULT hRetVal = E_INVALIDARG;
        if (IsEqualIID(IID_IUnknown, nIID))
        {
            AddRef();
            (*ppObject) = dynamic_cast<IUnknown *>(this);
            _ASSERTE(*ppObject);
            hRetVal = S_OK;
        }
        else if (IsEqualIID(__uuidof(IGEKUnknown), nIID))
        {
            AddRef();
            (*ppObject) = dynamic_cast<IGEKUnknown *>(this);
            _ASSERTE(*ppObject);
            hRetVal = S_OK;
        }

        return hRetVal;
    }

    STDMETHOD_(void, RegisterContext)           (THIS_ IGEKContext *pContext)
    {
        m_pContext = pContext;
    }

    STDMETHOD_(IGEKContext *, GetContext)       (THIS)
    {
        REQUIRE_RETURN(m_pContext, nullptr);
        return m_pContext;
    }

    STDMETHOD_(const IGEKContext *, GetContext) (THIS) const
    {
        REQUIRE_RETURN(m_pContext, nullptr);
        return m_pContext;
    }

    STDMETHOD_(IUnknown *, GetUnknown)          (THIS)
    {
        return dynamic_cast<IUnknown *>(this);
    }

    STDMETHOD_(const IUnknown *, GetUnknown)    (THIS) const
    {
        return dynamic_cast<const IUnknown *>(this);
    }

    template <typename CLASS>
    CLASS *GetClass(void)
    {
        return dynamic_cast<CLASS *>(GetUnknown());
    }

    template <typename CLASS>
    const CLASS *GetClass(void) const
    {
        return dynamic_cast<const CLASS *>(GetUnknown());
    }
};

template <typename RESULT>
class TGEKBaseEvent
{
public:
    virtual ~TGEKBaseEvent(void)
    {
    }

    virtual RESULT operator () (IGEKObserver *pObserver) const = 0;
};

template <typename INTERFACE>
class TGEKEvent : public TGEKBaseEvent<void>
{
private:
    std::function<void(INTERFACE *)> OnEvent;

public:
    TGEKEvent(std::function<void(INTERFACE *)> const &DoOnEvent) :
        OnEvent(DoOnEvent)
    {
    }

    virtual void operator () (IGEKObserver *pObserver) const
    {
        CComQIPtr<INTERFACE> spObserver(pObserver);
        if (spObserver)
        {
            OnEvent(spObserver);
        }
    }
};

template <typename INTERFACE>
class TGEKCheck : public TGEKBaseEvent<HRESULT>
{
private:
    std::function<HRESULT(INTERFACE *)> OnEvent;

public:
    TGEKCheck(std::function<HRESULT(INTERFACE *)> const &DoOnEvent) :
        OnEvent(DoOnEvent)
    {
    }

    virtual HRESULT operator () (IGEKObserver *pObserver) const
    {
        CComQIPtr<INTERFACE> spObserver(pObserver);
        if (spObserver)
        {
            return OnEvent(spObserver);
        }

        return E_FAIL;
    }
};

class CGEKObservable : public IGEKObservable
{
private:
    concurrency::concurrent_unordered_set<IGEKObserver *> m_aObservers;
    CComAutoCriticalSection m_kSection;

public:
    CGEKObservable(void)
    {
    }

    virtual ~CGEKObservable(void)
    {
    }

    STDMETHOD(AddObserver)      (THIS_ IGEKObserver *pObserver)
    {
        CComCritSecLock<CComAutoCriticalSection> kLock(m_kSection);

        HRESULT hRetVal = E_FAIL;
        auto pIterator = m_aObservers.find(pObserver);
        if (pIterator == m_aObservers.end())
        {
            m_aObservers.insert(pObserver);
            hRetVal = S_OK;
        }

        return hRetVal;
    }

    STDMETHOD(RemoveObserver)   (THIS_ IGEKObserver *pObserver)
    {
        CComCritSecLock<CComAutoCriticalSection> kLock(m_kSection);

        HRESULT hRetVal = E_FAIL;
        auto pIterator = m_aObservers.find(pObserver);
        if (pIterator != m_aObservers.end())
        {
            m_aObservers.unsafe_erase(pIterator);
            hRetVal = S_OK;
        }

        return hRetVal;
    }

    void SendEvent(const TGEKBaseEvent<void> &kEvent)
    {
        CComCritSecLock<CComAutoCriticalSection> kLock(m_kSection);
        for (auto &pObserver : m_aObservers)
        {
            kEvent(pObserver);
        }
    }

    HRESULT CheckEvent(const TGEKBaseEvent<HRESULT> &kEvent)
    {
        CComCritSecLock<CComAutoCriticalSection> kLock(m_kSection);

        HRESULT hRetVal = S_OK;
        for (auto &pObserver : m_aObservers)
        {
            hRetVal = kEvent(pObserver);
            if (FAILED(hRetVal))
            {
                break;
            }
        }

        return hRetVal;
    }

    static HRESULT AddObserver(IUnknown *pUnknown, IGEKObserver *pObserver)
    {
        HRESULT hRetVal = E_FAIL;
        IGEKObservable *pObservable = dynamic_cast<IGEKObservable *>(pUnknown);
        if (pObservable)
        {
            hRetVal = pObservable->AddObserver(pObserver);
        }

        return hRetVal;
    }

    static HRESULT RemoveObserver(IUnknown *pUnknown, IGEKObserver *pObserver)
    {
        HRESULT hRetVal = E_FAIL;
        IGEKObservable *pObservable = dynamic_cast<IGEKObservable *>(pUnknown);
        if (pObservable)
        {
            hRetVal = pObservable->RemoveObserver(pObserver);
        }

        return hRetVal;
    }
};

#define DECLARE_UNKNOWN(CLASS)                                                      \
    public:                                                                         \
        STDMETHOD(QueryInterface)(THIS_ REFIID nIID, void** ppObject);              \
        STDMETHOD_(ULONG, AddRef)(THIS);                                            \
        STDMETHOD_(ULONG, Release)(THIS);                                           \
    public:

#define BEGIN_INTERFACE_LIST(CLASS)                                                 \
    STDMETHODIMP_(ULONG) CLASS::AddRef(THIS)                                        \
    {                                                                               \
        return CGEKUnknown::AddRef();                                               \
    }                                                                               \
                                                                                    \
    STDMETHODIMP_(ULONG) CLASS::Release(THIS)                                       \
    {                                                                               \
        return CGEKUnknown::Release();                                              \
    }                                                                               \
                                                                                    \
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID nIID, LPVOID FAR *ppObject)     \
    {                                                                               \
        REQUIRE_RETURN(ppObject, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                        \
        if (IsEqualIID(INTERFACE_IID, nIID))                                        \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                   \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                          \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, nIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(nIID, ppObject);                        \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                    \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), ppObject);   \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                      \
        if (IsEqualIID(INTERFACE_IID, nIID))                                        \
        {                                                                           \
            return FUNCTION(nIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), nIID))                            \
        {                                                                           \
            return FUNCTION(nIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                       \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(nIID, ppObject)))                  \
        {                                                                           \
            return S_OK;                                                            \
        }

#define END_INTERFACE_LIST                                                          \
        (*ppObject) = nullptr;                                                      \
        return E_INVALIDARG;                                                        \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                  \
        return CGEKUnknown::QueryInterface(nIID, ppObject);                         \
    }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                         \
        return BASE_CLASS::QueryInterface(nIID, ppObject);                          \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                       \
        return FUNCTION(nIID, ppObject);                                            \
    }

#define REGISTER_CLASS(CLASS)                                                       \
HRESULT GEKCreateInstanceOf##CLASS##(IGEKUnknown **ppObject)                        \
{                                                                                   \
    REQUIRE_RETURN(ppObject, E_INVALIDARG);                                         \
                                                                                    \
    HRESULT hRetVal = E_OUTOFMEMORY;                                                \
    CComPtr<CLASS> spObject(new CLASS());                                           \
    if (spObject != nullptr)                                                        \
    {                                                                               \
        hRetVal = spObject->QueryInterface(IID_PPV_ARGS(ppObject));                 \
    }                                                                               \
                                                                                    \
    return hRetVal;                                                                 \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                             \
extern HRESULT GEKCreateInstanceOf##CLASS##(IGEKUnknown **ppObject);

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                          \
extern "C" __declspec(dllexport)                                                    \
HRESULT GEKGetModuleClasses(                                                        \
    std::unordered_map<CLSID, std::function<HRESULT (IGEKUnknown **)>> &aClasses,   \
    std::unordered_map<CLSID, std::vector<CLSID>> &aTypedClasses)                   \
{                                                                                   \
    CLSID kLastCLSID = GUID_NULL;

#define ADD_CONTEXT_CLASS(CLASSID, CLASS)                                           \
    if (aClasses.find(CLASSID) == aClasses.end())                                   \
    {                                                                               \
        aClasses[CLASSID] = GEKCreateInstanceOf##CLASS##;                           \
        kLastCLSID = CLASSID;                                                       \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        _ASSERTE(!"Duplicate class found in module: " #CLASSID);                    \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                      \
    aTypedClasses[TYPEID].push_back(kLastCLSID);

#define END_CONTEXT_SOURCE                                                          \
    return S_OK;                                                                    \
}

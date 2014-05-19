#pragma once

// Interfaces
#include "Public\IGEKObserver.h"
#include "Public\IGEKObservable.h"
#include "Public\IGEKUnknown.h"
#include "Public\IGEKContext.h"
#include <algorithm>
#include <list>
#include <map>

template <class CLASS>
class TGEKComPtr : public CComPtr<CLASS>
{
public:
    TGEKComPtr() throw()
    {
    }

    TGEKComPtr(CLASS *pObject) throw()
        : CComPtrBase<CLASS>(lp)
    {
    }

    TGEKComPtr(const CComPtr<CLASS>& pObject) throw()
        : CComPtrBase<CLASS>(pObject.p)
    {
    }

    TGEKComPtr(IGEKContext *pContext, REFCLSID kCLSID, HRESULT *pRetVal = nullptr) throw()
    {
        HRESULT hRetVal = pContext->CreateInstance(kCLSID, IID_PPV_ARGS(&p));
        if (pRetVal)
        {
            (*pRetVal) = hRetVal;
        }
    }
};

class CCompareGUID
{
public:
    bool operator()(const GUID &kValueA, const GUID &kValueB)
    {
        return memcmp(&kValueA, &kValueB, sizeof(GUID)) > 0 ? true : false;
    }
};

class CGEKUnknown : public IGEKUnknown
{
private:
    ULONG m_nRefCount;
    CComPtr<IGEKContext> m_spContext;

public:
    CGEKUnknown(void)
        : m_nRefCount(0)
    {
    }

    virtual ~CGEKUnknown(void)
    {
    }

    STDMETHOD_(ULONG, AddRef)               (THIS)
    {
        return InterlockedIncrement(&m_nRefCount);
    }

    STDMETHOD_(ULONG, Release)              (THIS)
    {
        LONG nRefCount = InterlockedDecrement(&m_nRefCount);
        if (nRefCount == 0)
        {
            Destroy();
            delete this;
        }

        return nRefCount;
    }

    STDMETHOD(QueryInterface)               (THIS_ REFIID rIID, LPVOID FAR *ppObject)
    {
        REQUIRE_RETURN(ppObject, E_INVALIDARG);

        HRESULT hRetVal = E_INVALIDARG;
        if (IsEqualIID(IID_IUnknown, rIID))
        {
            AddRef();
            (*ppObject) = dynamic_cast<IUnknown *>(this);
            _ASSERTE(*ppObject);
            hRetVal = S_OK;
        }
        else if (IsEqualIID(__uuidof(IGEKUnknown), rIID))
        {
            AddRef();
            (*ppObject) = dynamic_cast<IGEKUnknown *>(this);
            _ASSERTE(*ppObject);
            hRetVal = S_OK;
        }

        return hRetVal;
    }

    STDMETHOD(RegisterContext)              (THIS_ IGEKContext *pContext)
    {
        m_spContext = pContext;
        return Initialize();
    }

    STDMETHOD_(IGEKContext *, GetContext)   (THIS)
    {
        REQUIRE_RETURN(m_spContext, nullptr);
        return m_spContext;
    }

    STDMETHOD(Initialize)                   (THIS)
    {
        return S_OK;
    }

    STDMETHOD_(void, Destroy)               (THIS)
    {
    }
};

template <typename RESULT>
class CGEKEvent
{
public:
    virtual ~CGEKEvent(void)
    {
    }

    virtual RESULT operator () (IGEKObserver *pObserver) const = 0;
};

template <typename INTERFACE>
class TGEKEvent : public CGEKEvent<void>
{
private:
    std::function<void(INTERFACE *)> OnEvent;

public:
    TGEKEvent(std::function<void(INTERFACE *)> const &DoOnEvent)
        : OnEvent(DoOnEvent)
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
class TGEKCheck : public CGEKEvent<HRESULT>
{
private:
    std::function<HRESULT(INTERFACE *)> OnEvent;

public:
    TGEKCheck(std::function<HRESULT(INTERFACE *)> const &DoOnEvent)
        : OnEvent(DoOnEvent)
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
    std::list<IGEKObserver *> m_aObservers;

public:
    CGEKObservable(void)
    {
    }

    virtual ~CGEKObservable(void)
    {
    }

    STDMETHOD(AddObserver)      (THIS_ IGEKObserver *pObserver)
    {
        auto pIterator = std::find(m_aObservers.begin(), m_aObservers.end(), pObserver);
        if (pIterator == m_aObservers.end())
        {
            m_aObservers.push_back(pObserver);
            return S_OK;
        }

        return E_FAIL;
    }

    STDMETHOD(RemoveObserver)   (THIS_ IGEKObserver *pObserver)
    {
        auto pIterator = std::find(m_aObservers.begin(), m_aObservers.end(), pObserver);
        if (pIterator != m_aObservers.end())
        {
            m_aObservers.erase(pIterator);
            return S_OK;
        }

        return E_FAIL;
    }

    void SendEvent(const CGEKEvent<void> &kEvent)
    {
        for (auto &pObserver : m_aObservers)
        {
            kEvent(pObserver);
        }
    }

    HRESULT CheckEvent(const CGEKEvent<HRESULT> &kEvent)
    {
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
        IGEKObservable *pObservable = dynamic_cast<IGEKObservable *>(pUnknown);
        if (pObservable)
        {
            return pObservable->AddObserver(pObserver);
        }

        return E_FAIL;
    }

    static HRESULT RemoveObserver(IUnknown *pUnknown, IGEKObserver *pObserver)
    {
        IGEKObservable *pObservable = dynamic_cast<IGEKObservable *>(pUnknown);
        if (pObservable)
        {
            return pObservable->RemoveObserver(pObserver);
        }

        return E_FAIL;
    }
};

#define DECLARE_UNKNOWN(CLASS)                                                      \
    public:                                                                         \
        STDMETHOD(QueryInterface)(THIS_ REFIID rIID, void** ppObject);              \
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
    STDMETHODIMP CLASS::QueryInterface(THIS_ REFIID rIID, LPVOID FAR *ppObject)     \
    {                                                                               \
        REQUIRE_RETURN(ppObject, E_INVALIDARG);

#define INTERFACE_LIST_ENTRY(INTERFACE_IID, INTERFACE_CLASS)                        \
        if (IsEqualIID(INTERFACE_IID, rIID))                                        \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_COM(INTERFACE_CLASS)                                   \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), rIID))                            \
        {                                                                           \
            AddRef();                                                               \
            (*ppObject) = dynamic_cast<INTERFACE_CLASS *>(this);                    \
            _ASSERTE(*ppObject);                                                    \
            return S_OK;                                                            \
        }

#define INTERFACE_LIST_ENTRY_MEMBER(INTERFACE_IID, OBJECT)                          \
        if ((OBJECT) && IsEqualIID(INTERFACE_IID, rIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(rIID, ppObject);                        \
        }

#define INTERFACE_LIST_ENTRY_MEMBER_COM(INTERFACE_CLASS, OBJECT)                    \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), rIID))                            \
        {                                                                           \
            return (OBJECT)->QueryInterface(__uuidof(INTERFACE_CLASS), ppObject);   \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE(INTERFACE_IID, FUNCTION)                      \
        if (IsEqualIID(INTERFACE_IID, rIID))                                        \
        {                                                                           \
            return FUNCTION(rIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_DELEGATE_COM(INTERFACE_CLASS, FUNCTION)                \
        if (IsEqualIID(__uuidof(INTERFACE_CLASS), rIID))                            \
        {                                                                           \
            return FUNCTION(rIID, ppObject);                                        \
        }

#define INTERFACE_LIST_ENTRY_BASE(BASE_CLASS)                                       \
        if (SUCCEEDED(BASE_CLASS::QueryInterface(rIID, ppObject)))                  \
        {                                                                           \
            return S_OK;                                                            \
        }

#define END_INTERFACE_LIST                                                          \
        (*ppObject) = nullptr;                                                      \
        return E_INVALIDARG;                                                        \
    }

#define END_INTERFACE_LIST_UNKNOWN                                                  \
        return CGEKUnknown::QueryInterface(rIID, ppObject);                         \
    }

#define END_INTERFACE_LIST_BASE(BASE_CLASS)                                         \
        return BASE_CLASS::QueryInterface(rIID, ppObject);                          \
    }

#define END_INTERFACE_LIST_DELEGATE(FUNCTION)                                       \
        return FUNCTION(rIID, ppObject);                                            \
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

#define DECLARE_CONTEXT_SOURCE(SOURCENAME)                                              \
extern "C" __declspec(dllexport)                                                        \
HRESULT GEKGetModuleClasses(                                                            \
    std::map<CLSID, std::function<HRESULT (IGEKUnknown **)>, CCompareGUID> &aClasses,   \
    std::map<CStringW, CLSID> &aNamedClasses,                                           \
    std::map<CLSID, std::vector<CLSID>, CCompareGUID> &aTypedClasses)                   \
{                                                                                       \
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

#define ADD_CLASS_NAME(NAME)                                                        \
    if (aNamedClasses.find(NAME) == aNamedClasses.end())                            \
    {                                                                               \
        aNamedClasses[NAME] = kLastCLSID;                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        _ASSERTE(!"Duplicate name found in module: " #NAME);                        \
    }

#define ADD_CLASS_TYPE(TYPEID)                                                      \
    aTypedClasses[TYPEID].push_back(kLastCLSID);

#define END_CONTEXT_SOURCE                                                          \
    return S_OK;                                                                    \
}

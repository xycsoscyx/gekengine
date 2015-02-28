#pragma once

#include "GEKContext.h"
#include "GEKUtility.h"
#include "IGEKContext.h"
#include <unordered_map>
#include <list>

#include <concurrent_unordered_map.h>

class CGEKContext : public CGEKUnknown
                  , public CGEKObservable
                  , public IGEKContext
{
private:
    std::list<CStringW> m_aSearchPaths;

    std::list<HMODULE> m_aModules;
    std::unordered_map<CLSID, std::function<HRESULT(IGEKUnknown **ppObject)>> m_aClasses;
    std::unordered_map<CLSID, std::vector<CLSID>> m_aTypedClasses;
    std::unordered_map<CLSID, IUnknown *> m_aCache;

    std::unordered_map<CStringW, UINT32> m_aMetrics;

public:
    CGEKContext(void);
    virtual ~CGEKContext(void);
    DECLARE_UNKNOWN(CGEKContext);

    // IGEKContext
    STDMETHOD(AddSearchPath)                        (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)                           (THIS);
    STDMETHOD(CreateInstance)                       (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)                       (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate);
    STDMETHOD(AddCachedClass)                       (THIS_ REFCLSID kCLSID, IUnknown * const pObject);
    STDMETHOD(RemoveCachedClass)                    (THIS_ REFCLSID kCLSID);
    STDMETHOD_(IUnknown *, GetCachedClass)          (THIS_ REFCLSID kCLSID);
    STDMETHOD_(const IUnknown *, GetCachedClass)    (THIS_ REFCLSID kCLSID) const;
    STDMETHOD(AddCachedObserver)                    (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
    STDMETHOD(RemoveCachedObserver)                 (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
    STDMETHOD_(void, ResetMetrics)                  (THIS);
    STDMETHOD_(void, AdjustMetric)                  (THIS_ LPCWSTR pName, INT32 nCount);
    STDMETHOD_(void, ListMetrics)                   (THIS_ std::function<void(LPCWSTR, UINT32)> OnMetric);
};

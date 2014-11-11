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
    double m_nFrequency;
    std::map<CStringA, UINT32> m_aMetrics;

    std::list<CStringW> m_aSearchPaths;

    std::list<HMODULE> m_aModules;
    std::unordered_map<CLSID, std::function<HRESULT(IGEKUnknown **ppObject)>> m_aClasses;
    std::unordered_map<CStringW, CLSID> m_aNamedClasses;
    std::unordered_map<CLSID, std::vector<CLSID>> m_aTypedClasses;
    std::unordered_map<CLSID, IUnknown *> m_aCache;

public:
    CGEKContext(void);
    virtual ~CGEKContext(void);
    DECLARE_UNKNOWN(CGEKContext);

    // IGEKContext
    STDMETHOD_(double, GetTime)                     (THIS);
    STDMETHOD_(void, Log)                           (THIS_ LPCSTR pFile, UINT32 nLine, GEKLOGTYPE eType, LPCWSTR pMessage, ...);
    STDMETHOD(AddSearchPath)                        (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)                           (THIS);
    STDMETHOD(CreateInstance)                       (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateNamedInstance)                  (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)                       (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate);
    STDMETHOD(AddCachedClass)                       (THIS_ REFCLSID kCLSID, IUnknown * const pObject);
    STDMETHOD(RemoveCachedClass)                    (THIS_ REFCLSID kCLSID);
    STDMETHOD_(IUnknown *, GetCachedClass)          (THIS_ REFCLSID kCLSID);
    STDMETHOD_(const IUnknown *, GetCachedClass)    (THIS_ REFCLSID kCLSID) const;
    STDMETHOD(AddCachedObserver)                    (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
    STDMETHOD(RemoveCachedObserver)                 (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
};

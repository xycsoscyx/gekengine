#pragma once

#include "GEKContext.h"
#include "GEKUtility.h"
#include "IGEKContext.h"
#include <unordered_map>
#include <list>

#include <concurrent_unordered_map.h>

class CGEKContext : public CGEKObservable
                  , public IGEKContext
{
private:
    ULONG m_nRefCount;

    std::list<CStringW> m_aSearchPaths;

    std::list<HMODULE> m_aModules;
    std::unordered_map<CLSID, std::function<HRESULT(IGEKUnknown **ppObject)>> m_aClasses;
    std::unordered_map<CLSID, std::vector<CLSID>> m_aTypedClasses;

public:
    CGEKContext(void);
    virtual ~CGEKContext(void);

    // IUnknown
    STDMETHOD_(ULONG, AddRef)                   (THIS);
    STDMETHOD_(ULONG, Release)                  (THIS);
    STDMETHOD(QueryInterface)                   (THIS_ REFIID rIID, LPVOID FAR *ppObject);

    // IGEKContext
    STDMETHOD(AddSearchPath)                    (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD(CreateInstance)                   (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)                   (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate);
};

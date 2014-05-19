#pragma once

#include "GEKContext.h"
#include "IGEKContext.h"
#include <list>
#include <map>

class CGEKContext : public CGEKUnknown
                  , public IGEKContext
{
private:
    std::list<CStringW> m_aSearchPaths;

    std::list<HMODULE> m_aModules;
    std::map<CLSID, std::function<HRESULT (IGEKUnknown **ppObject)>, CCompareGUID> m_aClasses;
    std::map<CStringW, CLSID> m_aNamedClasses;
    std::map<CLSID, std::vector<CLSID>, CCompareGUID> m_aTypedClasses;

    std::map<CLSID, CComPtr<IUnknown>, CCompareGUID> m_aCache;

public:
    CGEKContext(void);
    virtual ~CGEKContext(void);
    DECLARE_UNKNOWN(CGEKContext);

    // IGEKContext
    STDMETHOD(AddSearchPath)            (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD(AddCacheClass)            (THIS_ REFCLSID kCLSID, IGEKUnknown *pObject);
    STDMETHOD(RemoveCacheClass)         (THIS_ REFCLSID kCLSID);
    STDMETHOD(CreateInstance)           (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateNamedInstance)      (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)           (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IGEKUnknown *pObject)> OnCreate);
};

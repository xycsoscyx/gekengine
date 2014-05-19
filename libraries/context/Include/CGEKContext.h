#pragma once

#include "GEKContext.h"
#include "IGEKContext.h"
#include <list>
#include <map>

class CGEKContext : public CGEKUnknown
                  , public CGEKObservable
                  , public IGEKContext
{
private:
    std::list<CStringW> m_aSearchPaths;

    std::list<HMODULE> m_aModules;
    std::map<CLSID, std::function<HRESULT (IUnknown **ppObject)>, CCompareGUID> m_aClasses;
    std::map<CStringW, CLSID> m_aNamedClasses;
    std::map<CLSID, std::vector<CLSID>, CCompareGUID> m_aTypedClasses;

public:
    CGEKContext(void);
    virtual ~CGEKContext(void);
    DECLARE_UNKNOWN(CGEKContext);

    // IGEKContext
    STDMETHOD(AddSearchPath)            (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD(CreateInstance)           (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateNamedInstance)      (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)           (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate);
    STDMETHOD(RegisterInstance)         (THIS_ IUnknown *pObject);
};

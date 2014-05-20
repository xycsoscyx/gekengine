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
    double m_nFrequency;

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
    STDMETHOD_(double, GetTime)             (THIS);
    STDMETHOD_(void, Log)                   (THIS_ LPCSTR pFile, UINT32 nLine, LPCWSTR pMessage, ...);
    STDMETHOD(AddSearchPath)                (THIS_ LPCWSTR pPath);
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD(CreateInstance)               (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateNamedInstance)          (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject);
    STDMETHOD(CreateEachType)               (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate);
    STDMETHOD(AddCachedClass)               (THIS_ REFCLSID kCLSID, IUnknown * const pObject);
    STDMETHOD(RemoveCachedClass)            (THIS_ REFCLSID kCLSID);
    STDMETHOD_(IUnknown *, GetCachedClass)  (THIS_ REFCLSID kCLSID);
    STDMETHOD(AddCachedObserver)            (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
    STDMETHOD(RemoveCachedObserver)         (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver);
};

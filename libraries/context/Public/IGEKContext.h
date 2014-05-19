#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE(IGEKUnknown);

DECLARE_INTERFACE_IID_(IGEKContext, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
{
    STDMETHOD(AddSearchPath)            (THIS_ LPCWSTR pPath) PURE;
    STDMETHOD(Initialize)               (THIS) PURE;

    STDMETHOD(AddCacheClass)            (THIS_ REFCLSID kCLSID, IGEKUnknown *pObject = nullptr) PURE;
    STDMETHOD(RemoveCacheClass)         (THIS_ REFCLSID kCLSID) PURE;

    STDMETHOD(CreateInstance)           (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateNamedInstance)      (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateEachType)           (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IGEKUnknown *pObject)> OnCreate) PURE;
};

HRESULT GEKCreateContext(IGEKContext **ppContext);

#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKContext, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
{
    STDMETHOD(AddSearchPath)            (THIS_ LPCWSTR pPath) PURE;
    STDMETHOD(Initialize)               (THIS) PURE;

    STDMETHOD(CreateInstance)           (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateNamedInstance)      (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateEachType)           (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate) PURE;

    STDMETHOD(RegisterInstance)         (THIS_ IUnknown *pObject) PURE;
};

DECLARE_INTERFACE_IID_(IGEKContextObserver, IGEKObserver, "81BA2FE9-4E6B-487F-A20C-A9C4F51965FC")
{
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject) PURE;
};

HRESULT GEKCreateContext(IGEKContext **ppContext);

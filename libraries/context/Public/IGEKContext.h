#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKContext, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
{
    STDMETHOD(AddSearchPath)                (THIS_ LPCWSTR pPath) PURE;
    STDMETHOD(Initialize)                   (THIS) PURE;

    STDMETHOD(CreateInstance)               (THIS_ REFCLSID kCLSID, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateNamedInstance)          (THIS_ LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(CreateEachType)               (THIS_ REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate) PURE;

    STDMETHOD(AddCachedClass)               (THIS_ REFCLSID kCLSID, IUnknown * const pObject) PURE;
    STDMETHOD(RemoveCachedClass)            (THIS_ REFCLSID kCLSID) PURE;
    STDMETHOD_(IUnknown *, GetCachedClass)  (THIS_ REFCLSID kCLSID) PURE;

    template <typename CLASS>
    CLASS *GetCachedClass(REFCLSID kCLSID)
    {
        IUnknown *pObject = GetCachedClass(kCLSID);
        if (pObject != NULL)
        {
            return dynamic_cast<CLASS *>(pObject);
        }

        return nullptr;
    }

    STDMETHOD(AddCachedObserver)            (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver) PURE;
    STDMETHOD(RemoveCachedObserver)         (THIS_ REFCLSID kCLSID, IGEKObserver *pObserver) PURE;
};

HRESULT GEKCreateContext(IGEKContext **ppContext);

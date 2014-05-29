#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKResource, IUnknown, "9CEFC12E-9D6E-4430-A6D6-12FEDF7FCF39")
{
    STDMETHOD(Load)             (THIS_ const UINT8 *pBuffer, LPCWSTR pParams) PURE;
};

DECLARE_INTERFACE_IID_(IGEKResourceProvider, IUnknown, "E6BF0DE1-B0B9-41E8-9E69-A39D43165D51")
{
    STDMETHOD(Load)             (THIS_ const UINT8 *pBuffer, LPCWSTR pParams, IUnknown **ppObject) PURE;
};

DECLARE_INTERFACE_IID_(IGEKResourceManager, IUnknown, "677D7619-0E15-4251-ACDC-6D3914B38C60")
{
    STDMETHOD(AddProvider)      (THIS_ LPCWSTR pName, IGEKResourceProvider *pProvider) PURE;
    STDMETHOD(Load)             (THIS_ LPCWSTR pName, LPCWSTR pParams, REFIID rIID, LPVOID FAR *ppObject) PURE;
    STDMETHOD(Load)             (THIS_ LPCWSTR pName, LPCWSTR pParams, std::function<void(IUnknown *)> OnLoaded) PURE;
};

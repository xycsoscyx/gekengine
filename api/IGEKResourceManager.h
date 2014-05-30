#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKResourceProvider, IUnknown, "5AFF886F-0556-48EF-8DEC-329E24AA6674")
{
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName, const UINT8 *pBuffer, UINT32 nBufferSize, IUnknown **ppObject) PURE;
};

DECLARE_INTERFACE_IID_(IGEKResourceManager, IUnknown, "60451D84-BCE6-4511-8090-694616788A20")
{
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName, std::function<void(IUnknown *)> OnReady) PURE;
};

#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKEngine, IUnknown, "06847C3A-70BE-4AAA-BABE-1D3741E5A1BC")
{
    STDMETHOD_(void, OnCommand)                 (THIS_ LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams) PURE;
};

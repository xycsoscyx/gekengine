#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKEngine, IUnknown, "06847C3A-70BE-4AAA-BABE-1D3741E5A1BC")
{
    STDMETHOD_(void, CaptureMouse)              (THIS_ bool bCapture) PURE;
    STDMETHOD_(bool, IsMouseCaptured)           (THIS) PURE;

    STDMETHOD_(void, OnCommand)                 (THIS_ LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams) PURE;
};

SYSTEM_USER(Engine, "104FAD32-0607-48F2-A725-7FA6220CAD59");
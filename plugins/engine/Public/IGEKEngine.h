#pragma once

#include "GEKContext.h"
#include <vector>

DECLARE_INTERFACE_IID_(IGEKEngine, IUnknown, "06847C3A-70BE-4AAA-BABE-1D3741E5A1BC")
{
    STDMETHOD_(void, OnMessage)                 (THIS_ LPCWSTR pMessage, ...) PURE;
    STDMETHOD_(void, OnCommand)                 (THIS_ const std::vector<CStringW> &aParams) PURE;
};

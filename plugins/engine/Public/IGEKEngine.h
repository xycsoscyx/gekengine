#pragma once

#include "GEKContext.h"
#include <vector>

enum GEKMESSAGETYPE
{
    GEKMESSAGE_NORMAL                           = 0,
    GEKMESSAGE_WARNING,
    GEKMESSAGE_ERROR,
    GEKMESSAGE_CRITICAL,
};

DECLARE_INTERFACE_IID_(IGEKEngine, IUnknown, "06847C3A-70BE-4AAA-BABE-1D3741E5A1BC")
{
    STDMETHOD_(void, ShowMessage)               (THIS_ GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...) PURE;
    STDMETHOD_(void, RunCommand)                (THIS_ LPCWSTR pCommand, const std::vector<CStringW> &aParams) PURE;
};

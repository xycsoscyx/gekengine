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

DECLARE_INTERFACE_IID_(IGEKConfigGroup, IUnknown, "1549B823-4875-48C9-B9A8-E4CEE4D69FB0")
{
    STDMETHOD_(LPCWSTR, GetText)                (THIS) PURE;

    STDMETHOD_(bool, HasGroup)                  (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(IGEKConfigGroup *, GetGroup)     (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(void, ListGroups)                (THIS_ std::function<void(LPCWSTR, IGEKConfigGroup*)> OnGroup) PURE;

    STDMETHOD_(bool, HasValue)                  (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(LPCWSTR, GetValue)               (THIS_ LPCWSTR pName, LPCWSTR pDefault = nullptr) PURE;
};

DECLARE_INTERFACE_IID_(IGEKEngine, IUnknown, "06847C3A-70BE-4AAA-BABE-1D3741E5A1BC")
{
    STDMETHOD_(IGEKConfigGroup *, GetConfig)    (THIS) PURE;
    STDMETHOD_(void, ShowMessage)               (THIS_ GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...) PURE;
    STDMETHOD_(void, RunCommand)                (THIS_ LPCWSTR pCommand, const std::vector<CStringW> &aParams) PURE;
};

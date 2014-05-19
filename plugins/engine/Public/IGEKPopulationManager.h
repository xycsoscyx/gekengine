#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKPopulationManager, IUnknown, "15D96F1B-3B38-4EAD-B62A-6AB3DFA2F1DD")
{
    STDMETHOD(LoadScene)                (THIS_ LPCWSTR pName, LPCWSTR pEntry) PURE;
    STDMETHOD_(void, Free)              (THIS) PURE;

    STDMETHOD_(void, OnInputEvent)      (THIS_ LPCWSTR pName, const GEKVALUE &kValue) PURE;

    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime) PURE;
};

SYSTEM_USER(PopulationManager, "0A920D46-6D72-4E90-9DC6-CD147A1775C7");

#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKPopulationManager, IUnknown, "15D96F1B-3B38-4EAD-B62A-6AB3DFA2F1DD")
{
    STDMETHOD(LoadScene)                (THIS_ LPCWSTR pName, LPCWSTR pEntry) PURE;
    STDMETHOD_(void, FreeScene)         (THIS) PURE;

    STDMETHOD(OnInputEvent)             (THIS_ LPCWSTR pName, const GEKVALUE &kValue) PURE;

    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime) PURE;
    STDMETHOD_(void, Render)            (THIS) PURE;
};

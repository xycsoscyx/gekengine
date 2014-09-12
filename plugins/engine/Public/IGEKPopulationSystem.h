#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKPopulationSystem, IUnknown, "15D96F1B-3B38-4EAD-B62A-6AB3DFA2F1DD")
{
    STDMETHOD(LoadSystems)              (THIS) PURE;
    STDMETHOD_(void, FreeSystems)       (THIS) PURE;

    STDMETHOD(Load)                     (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(void, Free)              (THIS) PURE;

    STDMETHOD_(void, Update)            (THIS_ float nGameTime, float nFrameTime) PURE;
};

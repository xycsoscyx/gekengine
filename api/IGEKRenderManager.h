#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "5226EB5A-DC03-465C-8393-0F947A0DDC24")
{
    STDMETHOD_(float2, GetScreenSize)           (THIS) const PURE;
};

DECLARE_INTERFACE_IID_(IGEKRenderManagerObserver, IGEKObserver, "16333226-FE0A-427D-A3EF-205486E1AD4D")
{
    STDMETHOD_(void, OnPreRender)               (THIS_ const GEKENTITYID &nViewID) { };
    STDMETHOD_(void, OnCull)                    (THIS_ const GEKENTITYID &nViewID) { };
    STDMETHOD_(void, OnPostRender)              (THIS_ const GEKENTITYID &nViewID) { };
};

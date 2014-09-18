#pragma once

#include "GEKContext.h"
#include "GEKShape.h"

DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "5226EB5A-DC03-465C-8393-0F947A0DDC24")
{
    STDMETHOD_(float2, GetScreenSize)           (THIS) const PURE;
    STDMETHOD_(const frustum &, GetFrustum)     (THIS) const PURE;
};

DECLARE_INTERFACE_IID_(IGEKRenderObserver, IGEKObserver, "16333226-FE0A-427D-A3EF-205486E1AD4D")
{
    STDMETHOD_(void, OnPreRender)               (THIS) { };
    STDMETHOD_(void, OnCullScene)               (THIS) { };
    STDMETHOD_(void, OnDrawScene)               (THIS_ IGEK3DVideoContext *pContext, UINT32 nVertexAttributes) { };
    STDMETHOD_(void, OnPostRender)              (THIS) { };
};

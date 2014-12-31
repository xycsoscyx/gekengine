#pragma once

#include "GEKContext.h"
#include "GEKShape.h"

enum GEKVERTEXATTRIBUTES
{
    GEK_VERTEX_POSITION     = 1 << 0,
    GEK_VERTEX_TEXCOORD     = 1 << 1,
    GEK_VERTEX_NORMAL       = 1 << 2,
};

DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "5226EB5A-DC03-465C-8393-0F947A0DDC24")
{
    STDMETHOD_(const frustum &, GetFrustum)     (THIS) const PURE;
};

DECLARE_INTERFACE_IID_(IGEKRenderObserver, IGEKObserver, "16333226-FE0A-427D-A3EF-205486E1AD4D")
{
    STDMETHOD_(void, OnRenderBegin)             (THIS) { };
    STDMETHOD_(void, OnCullScene)               (THIS) { };
    STDMETHOD_(void, OnDrawScene)               (THIS_ IGEK3DVideoContext *pContext, UINT32 nVertexAttributes) { };
    STDMETHOD_(void, OnRenderEnd)               (THIS) { };
    STDMETHOD_(void, OnRenderOverlay)           (THIS) { };
};

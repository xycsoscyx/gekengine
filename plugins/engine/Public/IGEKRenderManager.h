#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD_(void, Free)                  (THIS) PURE;

    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource) PURE;

    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppResource) PURE;

    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawLights)            (THIS_ std::function<void(void)> OnLightBatch) PURE;
    STDMETHOD_(void, DrawOverlay)           (THIS) PURE;

    STDMETHOD_(void, Render)                (THIS) PURE;
};

SYSTEM_USER(RenderManager, "32CB8ECF-720B-441D-8CF5-BDC8AAC67A3C");

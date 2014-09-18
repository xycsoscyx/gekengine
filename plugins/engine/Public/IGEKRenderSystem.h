#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoContextSystem);
DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderSystem, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD_(void, SetResource)           (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource) PURE;

    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppResource) PURE;
    STDMETHOD_(void, SetScreenTargets)      (THIS_ IGEK3DVideoContext *pContext, IUnknown *pDepthBuffer) PURE;

    STDMETHOD_(void, DrawScene)             (THIS_ IGEK3DVideoContext *pContext, UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawLights)            (THIS_ IGEK3DVideoContext *pContext, std::function<void(void)> OnLightBatch) PURE;
    STDMETHOD_(void, DrawOverlay)           (THIS_ IGEK3DVideoContext *pContext) PURE;

    STDMETHOD_(void, Render)                (THIS) PURE;
};

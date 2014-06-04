#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEKVideoContextSystem);

DECLARE_INTERFACE_IID_(IGEKEventResource, IUnknown, "1EBAD0F5-DE40-448F-A606-97D7D5062798")
{
    STDMETHOD_(void, Resize)                (THIS_ UINT32 nXSize, UINT32 nYSize) PURE;
    STDMETHOD_(void, OnEvent)               (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam) PURE;
};

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, bool bPersistent, IUnknown **ppResource) PURE;
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource) PURE;

    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppResource) PURE;

    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawLights)            (THIS_ std::function<void(void)> OnLightBatch) PURE;
    STDMETHOD_(void, DrawOverlay)           (THIS) PURE;

    STDMETHOD_(void, Render)                (THIS_ bool bUpdateScreen) PURE;
};

#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD_(void, BeginLoad)             (THIS) PURE;
    STDMETHOD_(void, EndLoad)               (THIS_ HRESULT hRetVal) PURE;

    STDMETHOD_(void, Free)                  (THIS) PURE;

    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource) PURE;

    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppResource) PURE;

    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawOverlay)           (THIS_ bool bPerLight) PURE;

    STDMETHOD(BeginFrame)                   (THIS) PURE;
    STDMETHOD_(const frustum &, GetFrustum) (THIS) PURE;
    STDMETHOD_(void, EndFrame)              (THIS) PURE;
};

SYSTEM_USER(RenderManager, "0A920D46-6D72-4E90-9DC6-CD147A1775C7");
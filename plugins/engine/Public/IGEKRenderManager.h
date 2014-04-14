#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKRenderManager, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD_(void, BeginLoad)     (THIS) PURE;
    STDMETHOD_(void, EndLoad)       (THIS_ HRESULT hRetVal) PURE;

    STDMETHOD(LoadWorld)            (THIS_ LPCWSTR pName, std::function<HRESULT(float3 *, IUnknown *)> OnStaticFace) PURE;
    STDMETHOD_(void, FreeWorld)     (THIS) PURE;

    STDMETHOD(LoadTexture)          (THIS_ LPCWSTR pName, IUnknown **ppTexture) PURE;
    STDMETHOD_(void, SetTexture)    (THIS_ UINT32 nStage, IUnknown *pTexture) PURE;

    STDMETHOD(GetBuffer)            (THIS_ LPCWSTR pName, IUnknown **ppTexture) PURE;
    STDMETHOD(GetDepthBuffer)       (THIS_ LPCWSTR pSource, IUnknown **ppBuffer) PURE;

    STDMETHOD_(void, DrawScene)     (THIS_ UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawOverlay)   (THIS_ bool bPerLight) PURE;

    STDMETHOD(BeginFrame)           (THIS) PURE;
    STDMETHOD_(void, EndFrame)      (THIS) PURE;
};

SYSTEM_USER(RenderManager, "0A920D46-6D72-4E90-9DC6-CD147A1775C7");
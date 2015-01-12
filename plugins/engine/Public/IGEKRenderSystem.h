#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoContextSystem);
DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderSystem, IUnknown, "77161A84-61C4-4C05-9550-4EEB74EF3CB1")
{
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;
    STDMETHOD(CreateBuffer)                 (THIS_ LPCWSTR pName, UINT32 nStride, UINT32 nCount) PURE;
    STDMETHOD(CreateBuffer)                 (THIS_ LPCWSTR pName, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount) PURE;
    STDMETHOD(CreateBuffer)                 (THIS_ LPCWSTR pName, UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat) PURE;

    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppResource) PURE;

    STDMETHOD_(void, SetDefaultRenderStates)(THIS_ IUnknown *pStates) PURE;
    STDMETHOD_(void, SetDefaultBlendStates) (THIS_ const float4 &nBlendFactor, UINT32 nMask, IUnknown *pStates) PURE;
    STDMETHOD_(void, EnableDefaultRenderStates) (THIS_ IGEK3DVideoContext *pContext) PURE;
    STDMETHOD_(void, EnableDefaultBlendStates)  (THIS_ IGEK3DVideoContext *pContext) PURE;

    STDMETHOD_(void, SetResource)           (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource) PURE;
    STDMETHOD_(void, SetScreenTargets)      (THIS_ IGEK3DVideoContext *pContext, IUnknown *pDepthBuffer) PURE;
    STDMETHOD_(void, FlipCurrentBuffers)    (THIS) PURE;

    STDMETHOD_(void, DrawScene)             (THIS_ IGEK3DVideoContext *pContext, UINT32 nAttributes) PURE;
    STDMETHOD_(void, DrawLights)            (THIS_ IGEK3DVideoContext *pContext, std::function<void(void)> OnLightBatch) PURE;
    STDMETHOD_(void, DrawOverlay)           (THIS_ IGEK3DVideoContext *pContext) PURE;

    STDMETHOD_(void, Render)                (THIS) PURE;
};

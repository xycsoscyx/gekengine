#pragma once

#include "GEKMath.h"
#include "GEKUtility.h"
#include <list>

DECLARE_INTERFACE(IGEKVideoTexture);

DECLARE_INTERFACE_IID_(IGEKRenderFilter, IUnknown, "9A3945DA-2E02-49A1-8107-FA08351A54A2")
{
    STDMETHOD(Load)                                         (THIS_ LPCWSTR pFileName) PURE;
    
    STDMETHOD_(UINT32, GetVertexAttributes)                 (THIS) PURE;
    STDMETHOD(GetBuffer)                                    (THIS_ LPCWSTR pName, IUnknown **ppTexture) PURE;
    STDMETHOD(GetDepthBuffer)                               (THIS_ IUnknown **ppBuffer) PURE;

    STDMETHOD_(IGEKVideoRenderStates *, GetRenderStates)    (THIS) PURE;
    STDMETHOD_(IGEKVideoBlendStates *, GetBlendStates)      (THIS) PURE;

    STDMETHOD_(void, Draw)                                  (THIS) PURE;
};

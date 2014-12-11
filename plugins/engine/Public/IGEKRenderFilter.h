#pragma once

#include "GEKMath.h"
#include "GEKUtility.h"
#include <list>

DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderFilter, IUnknown, "9A3945DA-2E02-49A1-8107-FA08351A54A2")
{
    STDMETHOD(Load)                                         (THIS_ LPCWSTR pFileName, const std::unordered_map<CStringA, CStringA> &aDefines) PURE;
    
    STDMETHOD_(UINT32, GetVertexAttributes)                 (THIS) PURE;

    STDMETHOD_(void, Draw)                                  (THIS_ IGEK3DVideoContext *pContext) PURE;
};

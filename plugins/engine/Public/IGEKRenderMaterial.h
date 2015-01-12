#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEKRenderSystem);
DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKRenderMaterial, IUnknown, "819CA201-F652-4183-B29D-BB71BB15810E")
{
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName) PURE;
    STDMETHOD_(bool, Enable)                (THIS_ IGEK3DVideoContext *pContext, LPCWSTR pLayer, const std::vector<CStringW> &aData) PURE;
};


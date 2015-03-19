#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoContext);
DECLARE_INTERFACE(IGEK3DVideoTexture);

DECLARE_INTERFACE_IID_(IGEKMaterialManager, IUnknown, "DCB2F842-2A3F-4E15-8263-B2D6F3A33786")
{
    STDMETHOD(LoadMaterial)             (THIS_ LPCWSTR pName, IUnknown **ppMaterial) PURE;
    STDMETHOD_(bool, EnableMaterial)    (THIS_ IGEK3DVideoContext *pContext, IUnknown *pMaterial) PURE;

    STDMETHOD(LoadTexture)              (THIS_ LPCWSTR pName, UINT32 nFlags, IGEK3DVideoTexture **ppTexture) PURE;
};

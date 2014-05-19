#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKMaterialManager, IUnknown, "DCB2F842-2A3F-4E15-8263-B2D6F3A33786")
{
    STDMETHOD(LoadMaterial)             (THIS_ LPCWSTR pName, IUnknown **ppMaterial) PURE;
    STDMETHOD(PrepareMaterial)          (THIS_ IUnknown *pMaterial) PURE;
    STDMETHOD_(bool, EnableMaterial)    (THIS_ IUnknown *pMaterial) PURE;
};

SYSTEM_USER(MaterialManager, "7DCCF48F-83E1-4322-8175-2D2261BFA401");

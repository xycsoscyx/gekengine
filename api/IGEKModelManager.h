#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKModelManager, IUnknown, "E44090AA-D287-444A-AACF-399015AB0D18")
{
    STDMETHOD(LoadCloud)            (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppCloud) PURE;

    STDMETHOD(LoadCollision)        (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppTriangles) PURE;

    STDMETHOD(LoadModel)            (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel) PURE;
};

SYSTEM_USER(ModelManager, "EE07DA62-6E01-4EF0-ADA3-07631DB6B4A4");

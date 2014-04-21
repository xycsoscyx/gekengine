#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKCollision, IUnknown, "D6140028-3F9C-4A1C-905A-4BEA905CA050")
{
    STDMETHOD_(UINT32, GetNumVertices)  (THIS) PURE;
    STDMETHOD_(float3 *, GetVertices)   (THIS) PURE;

    STDMETHOD_(UINT32, GetNumIndices)   (THIS) PURE;
    STDMETHOD_(UINT16 *, GetIndices)    (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKModelManager, IUnknown, "E44090AA-D287-444A-AACF-399015AB0D18")
{
    STDMETHOD(LoadCollision)        (THIS_ LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision) PURE;

    STDMETHOD(LoadModel)            (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel) PURE;
};

SYSTEM_USER(ModelManager, "EE07DA62-6E01-4EF0-ADA3-07631DB6B4A4");

#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKWorld, IUnknown, "086E00C2-EFA3-41FF-B3AA-ABCF10A28A99")
{
    STDMETHOD(Load)                 (THIS_ const UINT8 *pBuffer, std::function<void(float3 *, IUnknown *)> OnStaticFace) PURE;

    STDMETHOD_(aabb, GetAABB)       (THIS) PURE;

    STDMETHOD_(void, Prepare)       (THIS_ const frustum &nFrustum) PURE;
    STDMETHOD_(bool, IsVisible)     (THIS_ const aabb &nBox) PURE;
    STDMETHOD_(bool, IsVisible)     (THIS_ const obb &nBox) PURE;
    STDMETHOD_(bool, IsVisible)     (THIS_ const sphere &nSphere) PURE;
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes) PURE;
};

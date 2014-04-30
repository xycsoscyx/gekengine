#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKModel, IUnknown, "7A6B192B-0EA1-4944-BEF4-3C7871EE3FFD")
{
    struct INSTANCE
    {
        float4x4 m_nMatrix;
    };

    STDMETHOD_(aabb, GetAABB)       (THIS) PURE;

    STDMETHOD_(void, Prepare)       (THIS) PURE;
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes, const std::vector<INSTANCE> &aInstances) PURE;
};

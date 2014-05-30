#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKVideoBuffer);

DECLARE_INTERFACE_IID_(IGEKStaticFactory, IUnknown, "9BE6745D-DD62-428D-A031-02A0CF5B4229")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS) PURE;
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS) PURE;
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS) PURE;
};

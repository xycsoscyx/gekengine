#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKVideoBuffer);

DECLARE_INTERFACE_IID_(IGEKSpriteFactory, IUnknown, "C0EF3A45-43A0-42BC-9404-0E6A0E3DA6FD")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS) PURE;
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS) PURE;
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS) PURE;
};

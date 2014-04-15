#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKVideoVertexBuffer);

DECLARE_INTERFACE_IID_(IGEKStaticFactory, IUnknown, "9BE6745D-DD62-428D-A031-02A0CF5B4229")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS) PURE;
    STDMETHOD_(IGEKVideoVertexBuffer *, GetInstanceBuffer)      (THIS) PURE;
};

SYSTEM_USER(StaticFactory, "95333DEA-FFFE-4393-86EF-1F57233F7091");
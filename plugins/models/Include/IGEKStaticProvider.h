#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKVideoBuffer);

DECLARE_INTERFACE_IID_(IGEKStaticData, IUnknown, "8EA5376B-6A08-47F3-BE1D-C40F3B57CBD1")
{
    STDMETHOD(Load)                                             (THIS_ const UINT8 *pBuffer, UINT32 nBufferSize) PURE;
};

DECLARE_INTERFACE_IID_(IGEKStaticProvider, IUnknown, "9BE6745D-DD62-428D-A031-02A0CF5B4229")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS) PURE;
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS) PURE;
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS) PURE;
};

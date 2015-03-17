#pragma once

#include "GEKContext.h"

typedef UINT32 GEKRESOURCEID;
const GEKRESOURCEID GEKINVALIDRESOURCEID = 0;

DECLARE_INTERFACE_IID_(IGEKResourceSystem, IUnknown, "4680DA6E-2FD2-4918-A635-497C09A3969E")
{
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem) PURE;
};

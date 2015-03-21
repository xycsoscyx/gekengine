#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoSystem);
DECLARE_INTERFACE(IGEK3DVideoContext);
DECLARE_INTERFACE(IGEK3DVideoContextSystem);

typedef UINT32 GEKRESOURCEID;
const GEKRESOURCEID GEKINVALIDRESOURCEID = 0;

DECLARE_INTERFACE_IID_(IGEKResourceSystem, IUnknown, "4680DA6E-2FD2-4918-A635-497C09A3969E")
{
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem) PURE;

    STDMETHOD_(GEKRESOURCEID, LoadTexture)              (THIS_ LPCWSTR pFileName, UINT32 nFlags) PURE;
    STDMETHOD_(void, SetResource)                       (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nIndex, const GEKRESOURCEID &nResourceID) PURE;
    STDMETHOD_(void, SetUnorderedAccess)                (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID) PURE;
};

#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoContext);

DECLARE_INTERFACE_IID_(IGEKProgramManager, IUnknown, "915C2BA4-3FBD-4298-ACAD-CFB99260A9C7")
{
    STDMETHOD(LoadProgram)          (THIS_ LPCWSTR pName, IUnknown **ppProgram) PURE;
    STDMETHOD_(void, EnableProgram) (THIS_ IGEK3DVideoContext *pContext, IUnknown *pProgram) PURE;
};

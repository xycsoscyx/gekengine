#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKProgramManager, IUnknown, "915C2BA4-3FBD-4298-ACAD-CFB99260A9C7")
{
    STDMETHOD(LoadProgram)          (THIS_ LPCWSTR pName, IUnknown **ppProgram) PURE;
    STDMETHOD_(void, EnableProgram) (THIS_ IUnknown *pProgram) PURE;
};

SYSTEM_USER(ProgramManager, "47B3CA1D-CB1B-4B3B-8C4A-EC511435CC65");

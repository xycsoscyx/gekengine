#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKUnknown, IUnknown, "C66EB343-8E2B-47CE-BEF7-09F7B57AF7FD")
{
    STDMETHOD(Initialize)   (THIS)
    {
        return S_OK;
    }
};
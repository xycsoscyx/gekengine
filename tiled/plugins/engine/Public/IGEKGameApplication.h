#pragma once

#include <Windows.h>

DECLARE_INTERFACE_IID_(IGEKGameApplication, IUnknown, "4D347E4F-C94E-4422-B58C-C25BFEE61ABE")
{
    STDMETHOD_(void, Run)       (THIS) PURE;
};

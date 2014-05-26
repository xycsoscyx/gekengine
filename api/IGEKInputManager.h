#pragma once

#include "GEKContext.h"
#include "GEKVALUE.h"

DECLARE_INTERFACE_IID_(IGEKInputManager, IUnknown, "820222A3-3E23-4A13-8E5B-56B28225DB2A")
{
};

DECLARE_INTERFACE_IID_(IGEKInputObserver, IGEKObserver, "B1358995-5C9A-4177-AD73-D7F2DB0FD90B")
{
    STDMETHOD_(void, OnAction)          (THIS_ LPCWSTR pName, const GEKVALUE &kValue) { };
};

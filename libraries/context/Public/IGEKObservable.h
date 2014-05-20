#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKObserver, IUnknown, "445688B6-1BC4-463D-8727-13D89C95283D")
{
};

DECLARE_INTERFACE_IID_(IGEKObservable, IUnknown, "AC1B6840-905D-4303-A359-8990FFA1EAE1")
{
    STDMETHOD(AddObserver)              (THIS_ IGEKObserver *pObserver) PURE;
    STDMETHOD(RemoveObserver)           (THIS_ IGEKObserver *pObserver) PURE;
};

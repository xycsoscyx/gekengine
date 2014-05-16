#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE(IGEKObserver);

DECLARE_INTERFACE_IID_(IGEKObservable, IUnknown, "AC1B6840-905D-4303-A359-8990FFA1EAE1")
{
    STDMETHOD(AddObserver)      (THIS_ IGEKObserver *pObserver) PURE;
    STDMETHOD(RemoveObserver)   (THIS_ IGEKObserver *pObserver) PURE;
};

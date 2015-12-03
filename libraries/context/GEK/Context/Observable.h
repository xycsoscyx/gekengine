#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE(Observer);

    DECLARE_INTERFACE_IID(Observable, "AC1B6840-905D-4303-A359-8990FFA1EAE1") : virtual public IUnknown
    {
        STDMETHOD(addObserver)              (THIS_ Observer *observer) PURE;
        STDMETHOD(removeObserver)           (THIS_ Observer *observer) PURE;
    };
}; // namespace Gek

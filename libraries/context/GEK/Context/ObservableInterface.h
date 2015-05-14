#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE(ObserverInterface);

    DECLARE_INTERFACE_IID_(ObservableInterface, IUnknown, "AC1B6840-905D-4303-A359-8990FFA1EAE1")
    {
        STDMETHOD(addObserver)              (THIS_ ObserverInterface *observer) PURE;
        STDMETHOD(removeObserver)           (THIS_ ObserverInterface *observer) PURE;
    };
}; // namespace Gek

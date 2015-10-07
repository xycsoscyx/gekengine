#pragma once

#include <Windows.h>

namespace Gek
{
    namespace Observer
    {
        DECLARE_INTERFACE(Interface);
    }; // namespace Observer

    namespace Observable
    {
        DECLARE_INTERFACE_IID(Interface, "AC1B6840-905D-4303-A359-8990FFA1EAE1") : virtual public IUnknown
        {
            STDMETHOD(addObserver)              (THIS_ Observer::Interface *observer) PURE;
            STDMETHOD(removeObserver)           (THIS_ Observer::Interface *observer) PURE;
        };
    }; // namespace Observable
}; // namespace Gek

#pragma once

#include "GEK\Context\Observer.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(ActionObserver, "B1358995-5C9A-4177-AD73-D7F2DB0FD90B") : virtual public Observer
    {
        STDMETHOD_(void, onState)           (THIS_ LPCWSTR name, bool state) { };
        STDMETHOD_(void, onValue)           (THIS_ LPCWSTR name, float value) { };
    };
}; // namespace Gek

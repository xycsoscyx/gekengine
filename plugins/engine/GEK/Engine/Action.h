#pragma once

#include "GEK\Context\Observer.h"

namespace Gek
{
    struct ActionParam
    {
        union
        {
            bool state;
            float value;
        };

        ActionParam(bool state)
            : state(state)
        {
        }

        ActionParam(float value)
            : value(value)
        {
        }
    };

    DECLARE_INTERFACE_IID(ActionObserver, "B1358995-5C9A-4177-AD73-D7F2DB0FD90B") : virtual public Observer
    {
        STDMETHOD_(void, onAction)          (THIS_ LPCWSTR name, const ActionParam &param) { };
    };
}; // namespace Gek

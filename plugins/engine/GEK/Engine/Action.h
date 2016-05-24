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

    GEK_INTERFACE(ActionObserver)
    {
        virtual void onAction(LPCWSTR name, const ActionParam &param) = 0;
    };
}; // namespace Gek

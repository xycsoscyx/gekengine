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

    interface ActionObserver
    {
        void onAction(LPCWSTR name, const ActionParam &param) = default;
    };
}; // namespace Gek

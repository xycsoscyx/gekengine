#pragma once

#include "GEK\Context\Context.h"

namespace Gek
{
    GEK_INTERFACE(Observer)
    {
        virtual ~Observer(void) = default;

        Observer *getObserver(void)
        {
            return this;
        }
    };

    GEK_INTERFACE(Observable)
    {
        virtual void addObserver(Observer *observer) = 0;
        virtual void removeObserver(Observer *observer) = 0;
    };
}; // namespace Gek

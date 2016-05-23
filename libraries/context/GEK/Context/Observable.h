#pragma once

#include "GEK\Context\Context.h"

namespace Gek
{
    GEK_PREDECLARE(Observer);

    GEK_INTERFACE(Observable)
    {
    public:
        virtual void addObserver(Observer *observer) = 0;
        virtual void removeObserver(Observer *observer) = 0;
    };
}; // namespace Gek

#pragma once

#include <Windows.h>

namespace Gek
{
    GEK_PREDECLARE(ProcessorType);

    GEK_INTERFACE(Processor)
    {
        virtual void initialize(IUnknown *initializerContext) = 0;
    };
}; // namespace Gek

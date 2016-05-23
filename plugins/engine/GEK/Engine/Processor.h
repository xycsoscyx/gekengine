#pragma once

#include <Windows.h>

namespace Gek
{
    interface ProcessorType;

    interface Processor
    {
        void initialize(IUnknown *initializerContext);
    };
}; // namespace Gek

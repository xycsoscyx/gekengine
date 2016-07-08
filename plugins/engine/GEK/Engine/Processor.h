#pragma once

#include "GEK\Context\Context.h"

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Processor)
        {
            virtual ~Processor(void) = default;
        };
    }; // namespace Plugin
}; // namespace Gek

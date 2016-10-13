#pragma once

#include "GEK\Utility\Context.hpp"

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

#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Material)
        {
            GEK_START_EXCEPTIONS();

            virtual ~Material(void) = default;
        };
    }; // namespace Engine
}; // namespace Gek

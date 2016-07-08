#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(ResourceList)
        {
            virtual ~ResourceList(void) = default;
        };

        GEK_INTERFACE(Material)
        {
            virtual ResourceList * const getResourceList(void) const = 0;
        };
    }; // namespace Engine
}; // namespace Gek

#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"

namespace Gek
{
    GEK_PREDECLARE(ResourceList);

    GEK_INTERFACE(Material)
    {
        virtual ResourceList * const getResourceList(void) const = 0;
    };
}; // namespace Gek

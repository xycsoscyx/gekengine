#pragma once

#include "GEK\Engine\Population.h"
#include <typeindex>

#pragma warning(disable:4503)

namespace Gek
{
    interface ComponentType;

    interface Component
    {
        LPCWSTR getName(void) const;
        std::type_index getIdentifier(void) const;

        void *create(const Population::ComponentDefinition &componentData);
        void destroy(void *data);
    };
}; // namespace Gek

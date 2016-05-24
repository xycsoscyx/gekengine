#pragma once

#include "GEK\Engine\Population.h"
#include <typeindex>

#pragma warning(disable:4503)

namespace Gek
{
    interface ComponentType;

    GEK_INTERFACE(Component)
    {
        virtual const wchar_t *getName(void) const = 0;
        virtual std::type_index getIdentifier(void) const = 0;

        virtual void *create(const Population::ComponentDefinition &componentData) = 0;
        virtual void destroy(void *data) = 0;
    };
}; // namespace Gek

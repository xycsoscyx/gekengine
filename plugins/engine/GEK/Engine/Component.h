#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Population.h"
#include <typeindex>

#pragma warning(disable:4503)

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Component)
        {
            virtual const wchar_t * const getName(void) const = 0;
            virtual std::type_index getIdentifier(void) const = 0;

            virtual void *create(const Plugin::Population::ComponentDefinition &componentData) = 0;
            virtual void destroy(void *data) = 0;
        };
    }; // namespace Plugin
}; // namespace Gek

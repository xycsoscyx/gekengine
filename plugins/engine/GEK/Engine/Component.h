#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Population.h"
#include <typeindex>

#pragma warning(disable:4503)

namespace Gek
{
    GEK_INTERFACE(Component)
    {
        virtual const wchar_t * const getName(void) const = 0;
        virtual std::type_index getIdentifier(void) const = 0;

        virtual void *createData(const Population::ComponentDefinition &componentData) = 0;
        virtual void destroyData(void *data) = 0;
    };
}; // namespace Gek

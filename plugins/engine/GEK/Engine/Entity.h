#pragma once

#include "GEK\Context\Context.h"
#include <typeindex>

namespace Gek
{
    GEK_INTERFACE(Entity)
    {
        virtual const wchar_t *getName(void) const = 0;

        virtual bool hasComponent(const std::type_index &type) = 0;
        virtual void *getComponent(const std::type_index &type) = 0;

        template <typename CLASS>
        bool hasComponent(void)
        {
            return hasComponent(typeid(CLASS));
        }

        template<typename... ARGUMENTS>
        bool hasComponents(void)
        {
            std::vector<bool> hasComponents({ hasComponent<ARGUMENTS>()... });
            return std::find_if(hasComponents.begin(), hasComponents.end(), [&](bool hasComponent) -> bool
            {
                return !hasComponent;
            }) == hasComponents.end();
        }

        template <typename CLASS>
        CLASS &getComponent(void)
        {
            return *static_cast<CLASS *>(getComponent(typeid(CLASS)));
        }
    };
}; // namespace Gek

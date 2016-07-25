#pragma once

#include "GEK\Context\Context.h"
#include <typeindex>
#include <algorithm>

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Entity)
        {
            virtual bool hasComponent(const std::type_index &type) = 0;
            virtual void *getComponent(const std::type_index &type) = 0;

            template <typename CLASS>
            bool hasComponent(void)
            {
                return hasComponent(typeid(CLASS));
            }

            template<typename... PARAMETERS>
            bool hasComponents(void)
            {
                std::vector<bool> hasComponents({ hasComponent<PARAMETERS>()... });
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
    }; // namespace Plugin
}; // namespace Gek

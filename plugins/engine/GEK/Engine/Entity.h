#pragma once

#include <typeindex>

namespace Gek
{
    interface Entity
    {
        bool hasComponent(const std::type_index &type);
        LPVOID getComponent(const std::type_index &type);

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

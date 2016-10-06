#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Component.h"
#include <typeindex>
#include <algorithm>

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Entity)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(ComponentNotFound);

            virtual bool hasComponent(const std::type_index &type) const = 0;

			virtual Plugin::Component::Data *getComponent(const std::type_index &type) = 0;
			virtual const Plugin::Component::Data *getComponent(const std::type_index &type) const = 0;

            template <typename CLASS>
            bool hasComponent(void) const
            {
                return hasComponent(typeid(CLASS));
            }

            template<typename... PARAMETERS>
            bool hasComponents(void) const
            {
				std::vector<bool> hasComponentList({ hasComponent<PARAMETERS>()... });
				return (std::accumulate(hasComponentList.begin(), hasComponentList.end(), 0U) == hasComponentList.size());
            }

			template <typename CLASS>
			CLASS &getComponent(void)
			{
				return *static_cast<CLASS *>(getComponent(typeid(CLASS)));
			}

			template <typename CLASS>
			const CLASS &getComponent(void) const
			{
				return *static_cast<const CLASS *>(getComponent(typeid(CLASS)));
			}
		};
    }; // namespace Plugin

    namespace Editor
    {
        GEK_INTERFACE(Entity)
            : public Plugin::Entity
        {
            virtual std::unordered_map<std::type_index, std::unique_ptr<Plugin::Component::Data>> &getComponentsMap(void) = 0;
        };
    }; // namespace Editor
}; // namespace Gek

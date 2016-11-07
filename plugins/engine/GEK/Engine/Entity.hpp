/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 895bd642687b9b4b70b544b22192a2aa11a6e721 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 13 20:39:05 2016 +0000 $
#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Component.hpp"
#include <unordered_map>
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

            virtual ~Entity(void) = default;

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
				return (std::accumulate(std::begin(hasComponentList), std::end(hasComponentList), 0U) == hasComponentList.size());
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

    namespace Edit
    {
        GEK_INTERFACE(Entity)
            : public Plugin::Entity
        {
            virtual ~Entity(void) = default;

            using ComponentMap = std::unordered_map<std::type_index, std::unique_ptr<Plugin::Component::Data>>;
            virtual ComponentMap &getComponentMap(void) = 0;
        };
    }; // namespace Edit
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 895bd642687b9b4b70b544b22192a2aa11a6e721 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 13 20:39:05 2016 +0000 $
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/API/Component.hpp"
#include <typeindex>
#include <algorithm>
#include <numeric>

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Entity)
        {
            virtual ~Entity(void) = default;

            virtual bool hasComponent(Hash type) const = 0;

			virtual Plugin::Component::Data *getComponent(Hash type) = 0;
			virtual const Plugin::Component::Data *getComponent(Hash type) const = 0;

            template <typename COMPONENT>
            bool hasComponent(void) const
            {
				return hasComponent(COMPONENT::GetIdentifier());
            }

            template<typename... PARAMETERS>
            bool hasComponents(void) const
            {
				std::vector<bool> hasComponentList({ hasComponent<PARAMETERS>()... });
				return (std::accumulate(std::begin(hasComponentList), std::end(hasComponentList), 0U) == hasComponentList.size());
            }

			template <typename COMPONENT>
			COMPONENT &getComponent(void)
			{
				return *static_cast<COMPONENT *>(getComponent(COMPONENT::GetIdentifier()));
			}

			template <typename COMPONENT>
			const COMPONENT &getComponent(void) const
			{
				return *static_cast<const COMPONENT *>(getComponent(COMPONENT::GetIdentifier()));
			}
		};
    }; // namespace Plugin
}; // namespace Gek

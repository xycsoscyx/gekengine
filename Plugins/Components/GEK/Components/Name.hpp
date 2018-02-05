/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 68c94ed58445f7f7b11fb87263c60bc483158d4d $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 13:54:12 2016 +0000 $
#pragma once

#include "GEK/API/Component.hpp"
#include "GEK/API/Entity.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Name)
        {
			GEK_COMPONENT_DATA(Name);

			std::string name;
        };
    }; // namespace Components

    namespace Processor
    {
        GEK_INTERFACE(Name)
        {
            virtual ~Name(void) = default;

            virtual Plugin::Entity *getEntity(std::string const &name) = 0;
        };
    }; // namespace Processor
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Vector3.hpp"
#include "GEK\Engine\Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Model)
        {
            String name;
            String skin;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Components
}; // namespace Gek

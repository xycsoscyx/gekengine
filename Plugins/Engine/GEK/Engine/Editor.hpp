/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include <nod/nod.hpp>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(Editor)
        {
            virtual ~Editor(void) = default;

            nod::signal<void(Entity *entity, const std::type_index &type)> onModified;
        };
    }; // namespace Engine
}; // namespace Gek

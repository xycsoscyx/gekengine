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
#include <nano_signal_slot.hpp>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(Editor)
        {
            virtual ~Editor(void) = default;

            Nano::Signal<void(Entity *entity, const std::type_index &type)> onModified;
        };
    }; // namespace Engine
}; // namespace Gek

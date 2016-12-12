/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    GEK_INTERFACE(Window)
    {
        virtual ~Window(void) = default;
    };
}; // namespace Gek

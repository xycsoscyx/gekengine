/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Processor)
        {
            virtual void onInitialized(void) { };
        };
    }; // namespace Plugin
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Engine/Core.hpp"
#include <Windows.h>

namespace Gek
{
    GEK_INTERFACE(Application)
    {
        virtual ~Application(void) = default;

        virtual bool update(void) = 0;
    };
}; // namespace Gek

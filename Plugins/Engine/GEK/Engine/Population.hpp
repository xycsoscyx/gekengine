/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/API/Editor.hpp"

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Population)
            : public Edit::Population
        {
            virtual void reset(void) = 0;

            virtual void update(float frameTime) = 0;
        };
    }; // namespace Plugin
}; // namespace Gek
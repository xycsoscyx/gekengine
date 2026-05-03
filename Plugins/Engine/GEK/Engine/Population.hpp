/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "API/Engine/Editor.hpp"
#include "GEK/Utility/Context.hpp"

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
    }; // namespace Engine
}; // namespace Gek
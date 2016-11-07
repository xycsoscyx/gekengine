/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\System\VideoDevice.hpp"

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Visual)
        {
            GEK_ADD_EXCEPTION(InvalidElementType);
            GEK_ADD_EXCEPTION(MissingParameters);
            
            virtual ~Visual(void) = default;

            virtual void enable(Video::Device::Context *context) = 0;
        };
    }; // namespace Engine
}; // namespace Gek

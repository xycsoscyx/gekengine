/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/System/RenderDevice.hpp"

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Visual)
        {
            virtual ~Visual(void) = default;

			virtual std::string_view getName(void) const = 0;

            virtual void enable(Render::Device::Context *context) = 0;
        };
    }; // namespace Engine
}; // namespace Gek

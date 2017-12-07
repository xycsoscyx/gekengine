/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Vector4.hpp"
#include "GEK/API/Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Color)
        {
            Math::Float4 value = Math::Float4::White;
        };
    }; // namespace Components
}; // namespace Gek

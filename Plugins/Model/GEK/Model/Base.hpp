/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/Shapes/AlignedBox.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Model)
        {
            std::string name;
        };
    }; // namespace Components

    namespace Processor
    {
        GEK_INTERFACE(Model)
        {
            virtual Shapes::AlignedBox getBoundingBox(std::string const &modelName) = 0;
        };
    }; // namespace Processor
}; // namespace Gek

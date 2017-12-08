/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector3.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct Sphere
        {
        public:
            Math::Float3 position;
            float radius;

        public:
            Sphere(void) noexcept;
            Sphere(const Sphere &sphere) noexcept;
            Sphere(Math::Float3 const &position, float radius) noexcept;

            Sphere &operator = (const Sphere &sphere) noexcept;
        };
    }; // namespace Shapes
}; // namespace Gek

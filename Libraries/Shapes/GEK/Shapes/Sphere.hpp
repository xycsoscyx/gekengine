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
            Sphere(void);
            Sphere(const Sphere &sphere);
            Sphere(Math::Float3 const &position, float radius);

            Sphere &operator = (const Sphere &sphere);
        };
    }; // namespace Shapes
}; // namespace Gek

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
        struct Plane;

        struct Sphere
        {
        public:
            Math::Float3 position;
            float radius;

        public:
            Sphere(void);
            Sphere(const Sphere &sphere);
            Sphere(const Math::Float3 &position, float radius);

            Sphere &operator = (const Sphere &sphere);

            int getPosition(const Plane &plane) const;
        };
    }; // namespace Shapes
}; // namespace Gek

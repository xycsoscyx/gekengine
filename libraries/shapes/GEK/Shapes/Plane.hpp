/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Math\Float4.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct Plane
        {
        public:
            union
            {
                struct { float a, b, c, d; };
                struct { Math::Float3 normal; float distance; };
                struct { Math::Float4 vector; };
                struct { float data[4]; };
            };

        public:
            Plane(void);
            Plane(float a, float b, float c, float d);
            Plane(const Math::Float3 &normal, float distance);
            Plane(const Math::Float3 &pointA, const Math::Float3 &pointB, const Math::Float3 &pointC);
            Plane(const Math::Float3 &normal, const Math::Float3 &pointOnPlane);

            inline Plane &operator = (const Plane &plane)
            {
                vector = plane.vector;
                return (*this);
            }

            void normalize(void);

            float getDistance(const Math::Float3 &point) const;
        };
    }; // namespace Shapes
}; // namespace Gek

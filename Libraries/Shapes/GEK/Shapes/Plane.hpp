/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"

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
            Plane(Math::Float3 const &normal, float distance);
            Plane(Math::Float3 const &pointA, Math::Float3 const &pointB, Math::Float3 const &pointC);
            Plane(Math::Float3 const &normal, Math::Float3 const &pointOnPlane);

            Plane &operator = (Plane const &plane);

            void normalize(void);

            float getDistance(Math::Float3 const &point) const;
        };
    }; // namespace Shapes
}; // namespace Gek

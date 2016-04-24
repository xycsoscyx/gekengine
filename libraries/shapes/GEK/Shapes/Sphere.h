#pragma once

#include "GEK\Math\Float3.h"

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

            Sphere operator = (const Sphere &sphere);

            int getPosition(const Plane &plane) const;
        };
    }; // namespace Shapes
}; // namespace Gek

#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        struct Sphere
        {
        public:
            Math::Float3 position;
            float radius;

        public:
            Sphere(void)
                : radius(0.0f)
            {
            }

            Sphere(const Sphere &sphere)
                : position(sphere.position)
                , radius(sphere.radius)
            {
            }

            Sphere(const Math::Float3 &position, float radius)
                : position(position)
                , radius(radius)
            {
            }

            Sphere operator = (const Sphere &sphere)
            {
                position = sphere.position;
                radius = sphere.radius;
                return (*this);
            }

            int getPosition(const Plane &plane) const
            {
                float distance = plane.getDistance(position);
                if (distance < -radius)
                {
                    return -1;
                }
                else if (distance > radius)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        };
    }; // namespace Shape
}; // namespace Gek

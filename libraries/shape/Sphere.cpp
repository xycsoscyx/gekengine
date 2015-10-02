#include "GEK\Shape\Sphere.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        Sphere::Sphere(void)
            : radius(0.0f)
        {
        }

        Sphere::Sphere(const Sphere &sphere)
            : position(sphere.position)
            , radius(sphere.radius)
        {
        }

        Sphere::Sphere(const Math::Float3 &position, float radius)
            : position(position)
            , radius(radius)
        {
        }

        Sphere Sphere::operator = (const Sphere &sphere)
        {
            position = sphere.position;
            radius = sphere.radius;
            return (*this);
        }

        int Sphere::getPosition(const Plane &plane) const
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
    }; // namespace Shape
}; // namespace Gek

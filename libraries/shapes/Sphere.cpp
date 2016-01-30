#include "GEK\Shapes\Sphere.h"
#include "GEK\Shapes\Plane.h"

namespace Gek
{
    namespace Shapes
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
    }; // namespace Shapes
}; // namespace Gek

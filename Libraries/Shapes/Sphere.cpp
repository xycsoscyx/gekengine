#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Shapes/Plane.hpp"

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

        Sphere::Sphere(Math::Float3 const &position, float radius)
            : position(position)
            , radius(radius)
        {
        }

        Sphere &Sphere::operator = (const Sphere &sphere)
        {
            position = sphere.position;
            radius = sphere.radius;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

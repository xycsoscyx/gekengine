#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Shapes/Plane.hpp"

namespace Gek
{
    namespace Shapes
    {
        Sphere::Sphere(void) noexcept
            : radius(0.0f)
        {
        }

        Sphere::Sphere(const Sphere &sphere) noexcept
            : position(sphere.position)
            , radius(sphere.radius)
        {
        }

        Sphere::Sphere(Math::Float3 const &position, float radius) noexcept
            : position(position)
            , radius(radius)
        {
        }

        Sphere &Sphere::operator = (const Sphere &sphere) noexcept
        {
            position = sphere.position;
            radius = sphere.radius;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

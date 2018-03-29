#include "GEK/Shapes/Ray.hpp"
#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Math/Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        Ray::Ray(void) noexcept
        {
        }

        Ray::Ray(Math::Float3 const &origin, Math::Float3 const &normal) noexcept
            : origin(origin)
            , normal(normal)
        {
        }

        Ray::Ray(Ray const &ray) noexcept
            : origin(ray.origin)
            , normal(ray.normal)
        {
        }

        Ray &Ray::operator = (Ray const &ray) noexcept
        {
            origin = ray.origin;
            normal = ray.normal;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

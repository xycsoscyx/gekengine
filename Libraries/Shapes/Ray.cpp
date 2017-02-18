#include "GEK/Shapes/Ray.hpp"
#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Math/Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        Ray::Ray(void)
        {
        }

        Ray::Ray(Math::Float3 const &origin, Math::Float3 const &normal)
            : origin(origin)
            , normal(normal)
        {
        }

        Ray::Ray(const Ray &ray)
            : origin(ray.origin)
            , normal(ray.normal)
        {
        }

        Ray &Ray::operator = (const Ray &ray)
        {
            origin = ray.origin;
            normal = ray.normal;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

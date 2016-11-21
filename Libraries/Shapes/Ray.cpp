#include "GEK\Shapes\Ray.hpp"
#include "GEK\Shapes\OrientedBox.hpp"
#include "GEK\Math\Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        Ray::Ray(void)
        {
        }

        Ray::Ray(const Math::Float3 &origin, const Math::Float3 &normal)
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

        float Ray::getDistance(const OrientedBox &orientedBox) const
        {
            float minimum(0.0f);
            float maximum(Math::Infinity);
            Math::Float3 positionDelta(orientedBox.matrix.translation.xyz - origin);
            for (int axis = 0; axis < 3; ++axis)
            {
                float axisAngle = orientedBox.matrix.rows[axis].xyz.dot(positionDelta);
                float rayAngle = normal.dot(orientedBox.matrix.rows[axis].xyz);
                if (std::abs(rayAngle) > Math::Epsilon)
                {
                    float positionDelta1 = ((axisAngle - orientedBox.halfsize[axis]) / rayAngle);
                    float positionDelta2 = ((axisAngle + orientedBox.halfsize[axis]) / rayAngle);
                    if (positionDelta1 > positionDelta2)
                    {
                        float positionDeltaSwap = positionDelta1;
                        positionDelta1 = positionDelta2;
                        positionDelta2 = positionDeltaSwap;
                    }

                    if (positionDelta2 < maximum)
                    {
                        maximum = positionDelta2;
                    }

                    if (positionDelta1 > minimum)
                    {
                        minimum = positionDelta1;
                    }

                    if (maximum < minimum)
                    {
                        return float(-1);
                    }
                }
                else
                {
                    if ((-axisAngle - orientedBox.halfsize[axis]) > 0.0f ||
                        (-axisAngle + orientedBox.halfsize[axis]) < 0.0f)
                    {
                        return float(-1);
                    }
                }
            }

            return minimum;
        }
    }; // namespace Shapes
}; // namespace Gek

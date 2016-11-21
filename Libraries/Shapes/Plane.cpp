#include "GEK\Shapes\Plane.hpp"

namespace Gek
{
    namespace Shapes
    {
        Plane::Plane(void)
            : normal(0.0f, 0.0f, 0.0f)
            , distance(0.0f)
        {
        }

        Plane::Plane(float a, float b, float c, float d)
            : normal(a, b, c)
            , distance(d)
        {
        }

        Plane::Plane(const Math::Float3 &normal, float distance)
            : normal(normal)
            , distance(distance)
        {
        }

        Plane::Plane(const Math::Float3 &pointA, const Math::Float3 &pointB, const Math::Float3 &pointC)
        {
            Math::Float3 sideA(pointB - pointA);
            Math::Float3 sideB(pointC - pointA);
            Math::Float3 sideC(sideA.cross(sideB));

            normal = sideC.getNormal();
            distance = -normal.dot(pointA);
        }

        Plane::Plane(const Math::Float3 &normal, const Math::Float3 &pointOnPlane)
            : normal(normal)
            , distance(-normal.dot(pointOnPlane))
        {
        }

        void Plane::normalize(void)
        {
            vector *= (1.0f / normal.getMagnitude());
        }

        float Plane::getDistance(const Math::Float3 &point) const
        {
            // +distance because we negated it when creating
            return (normal.dot(point) + distance);
        }
    }; // namespace Shapes
}; // namespace Gek

#include "GEK/Shapes/Plane.hpp"

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

        Plane::Plane(Math::Float3 const &normal, float distance)
            : normal(normal)
            , distance(distance)
        {
        }

        Plane::Plane(Math::Float3 const &pointA, Math::Float3 const &pointB, Math::Float3 const &pointC)
        {
            Math::Float3 sideA(pointB - pointA);
            Math::Float3 sideB(pointC - pointA);
            Math::Float3 sideC(sideA.cross(sideB));

            normal = sideC.getNormal();
            distance = -normal.dot(pointA);
        }

        Plane::Plane(Math::Float3 const &normal, Math::Float3 const &pointOnPlane)
            : normal(normal)
            , distance(-normal.dot(pointOnPlane))
        {
        }

        Plane &Plane::operator = (Plane const &plane)
        {
            vector = plane.vector;
            return (*this);
        }

        void Plane::normalize(void)
        {
            vector *= (1.0f / normal.getLength());
        }

        float Plane::getDistance(Math::Float3 const &point) const
        {
            // +distance because we negated it when creating
            return (normal.dot(point) + distance);
        }
    }; // namespace Shapes
}; // namespace Gek

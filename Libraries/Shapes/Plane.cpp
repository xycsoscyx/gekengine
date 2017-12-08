#include "GEK/Shapes/Plane.hpp"

namespace Gek
{
    namespace Shapes
    {
        Plane::Plane(void) noexcept
            : normal(0.0f, 0.0f, 0.0f)
            , distance(0.0f)
        {
        }

        Plane::Plane(float a, float b, float c, float d) noexcept
            : normal(a, b, c)
            , distance(d)
        {
        }

        Plane::Plane(Math::Float3 const &normal, float distance) noexcept
            : normal(normal)
            , distance(distance)
        {
        }

        Plane::Plane(Math::Float3 const &pointA, Math::Float3 const &pointB, Math::Float3 const &pointC) noexcept
        {
            Math::Float3 sideA(pointB - pointA);
            Math::Float3 sideB(pointC - pointA);
            Math::Float3 sideC(sideA.cross(sideB));

            normal = sideC.getNormal();
            distance = -normal.dot(pointA);
        }

        Plane::Plane(Math::Float3 const &normal, Math::Float3 const &pointOnPlane) noexcept
            : normal(normal)
            , distance(-normal.dot(pointOnPlane))
        {
        }

        Plane &Plane::operator = (Plane const &plane) noexcept
        {
            vector = plane.vector;
            return (*this);
        }

        void Plane::normalize(void) noexcept
        {
            vector *= (1.0f / normal.getLength());
        }

        float Plane::getDistance(Math::Float3 const &point) const noexcept
        {
            // +distance because we negated it when creating
            return (normal.dot(point) + distance);
        }

        Math::Float3 Plane::getIntersection(Math::Float3 const &a, Math::Float3 const &b) const noexcept
        {
            Math::Float3 ba = b - a;
            float nDotA = normal.dot(a);
            float nDotBA = normal.dot(ba);
            return a + (((distance - nDotA) / nDotBA) * ba);
        }
    }; // namespace Shapes
}; // namespace Gek

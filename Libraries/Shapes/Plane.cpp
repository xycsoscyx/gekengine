#include "GEK/Shapes/Plane.hpp"

namespace Gek
{
    namespace Shapes
    {
        Plane::Plane(void) noexcept
            : a(0.0f)
            , b(0.0f)
            , c(0.0f)
            , d(0.0f)
        {
        }

        Plane::Plane(float a, float b, float c, float d) noexcept
            : a(a)
            , b(b)
            , c(c)
            , d(d)
        {
        }

        Plane::Plane(Math::Float3 const &normal, float distance) noexcept
            : vector(normal, distance)
        {
        }

        Plane::Plane(Math::Float3 const &pointA, Math::Float3 const &pointB, Math::Float3 const &pointC) noexcept
        {
            Math::Float3 sideA(pointB - pointA);
            Math::Float3 sideB(pointC - pointA);
            Math::Float3 sideC(sideA.cross(sideB));

            vector.xyz() = sideC.getNormal();
            vector.w = -vector.xyz().dot(pointA);
        }

        Plane::Plane(Math::Float3 const &normal, Math::Float3 const &pointOnPlane) noexcept
            : vector(normal, -normal.dot(pointOnPlane))
        {
        }

        Plane &Plane::operator = (Plane const &plane) noexcept
        {
            vector = plane.vector;
            return (*this);
        }

        void Plane::normalize(void) noexcept
        {
            vector *= (1.0f / vector.xyz().getLength());
        }

        float Plane::getDistance(Math::Float3 const &point) const noexcept
        {
            // +distance because we negated it when creating
            return (vector.xyz().dot(point) + vector.w);
        }

        Math::Float3 Plane::getIntersection(Math::Float3 const &a, Math::Float3 const &b) const noexcept
        {
            Math::Float3 ba(b - a);
            float NdotA = vector.xyz().dot(a);
            float NdotBA = vector.xyz().dot(ba);
            return a + (((vector.w - NdotA) / NdotBA) * ba);
        }
    }; // namespace Shapes
}; // namespace Gek

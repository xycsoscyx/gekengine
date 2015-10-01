#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Shape
    {
        struct Plane
        {
        public:
            union
            {
                struct { float a, b, c, d; };
                struct { Math::Float3 normal; float distance; };
                struct { Math::Float4 vector; };
                struct { float data[4]; };
            };

        public:
            Plane(void)
                : normal(0.0f, 0.0f, 0.0f)
                , distance(0.0f)
            {
            }

            Plane(float a, float b, float c, float d)
                : normal(a, b, c)
                , distance(d)
            {
            }

            Plane(const Math::Float3 &normal, float distance)
                : normal(normal)
                , distance(distance)
            {
            }

            Plane(const Math::Float3 &pointA, const Math::Float3 &pointB, const Math::Float3 &pointC)
            {
                Math::Float3 sideA(pointB - pointA);
                Math::Float3 sideB(pointC - pointA);
                Math::Float3 sideC(sideA.cross(sideB));

                normal = sideC.getNormal();
                distance = -normal.dot(pointA);
            }

            Plane(const Math::Float3 &normal, const Math::Float3 &pointOnPlane)
                : normal(normal)
                , distance(-normal.dot(pointOnPlane))
            {
            }

            void normalize(void)
            {
                vector *= (1.0f / normal.getLength());
            }

            float getDistance(const Math::Float3 &point) const
            {
                return (normal.dot(point) + distance);
            }
        };
    }; // namespace Shape
}; // namespace Gek

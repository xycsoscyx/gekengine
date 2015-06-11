#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        template <typename TYPE>
        struct BaseSphere
        {
        public:
            Math::BaseVector3<TYPE> position;
            TYPE radius;

        public:
            BaseSphere(void)
                : radius(0.0f)
            {
            }

            BaseSphere(const BaseSphere<TYPE> &nSphere)
                : position(nSphere.position)
                , radius(nSphere.radius)
            {
            }

            BaseSphere(const Math::BaseVector3<TYPE> &nPosition, TYPE nRadius)
                : position(nPosition)
                , radius(nRadius)
            {
            }

            BaseSphere operator = (const BaseSphere<TYPE> &sphere)
            {
                position = sphere.position;
                radius = sphere.radius;
                return (*this);
            }

            int getPosition(const BasePlane<TYPE> &plane) const
            {
                TYPE distance = plane.getDistance(position);
                if (distance < -radius)
                {
                    return -1;
                }
                else if (distance > radius)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        };

        typedef BaseSphere<float> Sphere;
    }; // namespace Shape
}; // namespace Gek

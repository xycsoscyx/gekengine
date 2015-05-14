#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Utility\Plane.h"

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
                : radius(TYPE(0))
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

            BaseSphere operator = (const BaseSphere<TYPE> &nSphere)
            {
                position = nBox.position;
                radius = nBox.radius;
                return (*this);
            }

            int getPosition(const BasePlane<TYPE> &nPlane) const
            {
                TYPE nDistance = nPlane.Distance(position);
                if (nDistance < -radius)
                {
                    return -1;
                }
                else if (nDistance > radius)
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

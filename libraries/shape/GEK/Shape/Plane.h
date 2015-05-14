#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Shape
    {
        template <typename TYPE>
        struct BasePlane : public Math::BaseVector4<TYPE>
        {
        public:
            BasePlane(void)
                : x(0)
                , y(0)
                , z(0)
                , w(0)
            {
            }

            BasePlane(const Math::BaseVector4<TYPE> &vector)
                : Math::BaseVector4(vector)
            {
            }

            BasePlane(TYPE a, TYPE b, TYPE c, TYPE d)
                : Math::BaseVector4(a, b, c, d)
            {
            }

            BasePlane(const Math::BaseVector3<TYPE> &normal, TYPE distance)
                : Math::BaseVector4(normal, distance)
            {
            }

            BasePlane(const Math::BaseVector3<TYPE> &pointA, const Math::BaseVector3<TYPE> &pointB, const Math::BaseVector3<TYPE> &pointC)
            {
                Math::BaseVector3 sideA(pointB - pointA);
                Math::BaseVector3 sideB(pointC - pointA);
                Math::BaseVector3 sideC(sideA.Cross(sideB));

                normal = sideC.getNormal();
                distance = -normal.dot(pointA);
            }

            BasePlane(const Math::BaseVector3<TYPE> &nNormal, const Math::BaseVector3<TYPE> &nPointOnPlane)
            {
                normal = nNormal;
                distance = -normal.Dot(nPointOnPlane);
            }

            void normalize(void)
            {
                (*this) *= (TYPE(1) / normal.getLength());
            }

            TYPE getDistance(const Math::BaseVector3<TYPE> &point) const
            {
                return (normal.Dot(point) + distance);
            }
        };

        typedef BasePlane<float> Plane;
    }; // namespace Shape
}; // namespace Gek

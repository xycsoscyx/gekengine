#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Shape\OrientedBox.h"

namespace Gek
{
    namespace Shape
    {
        template <typename TYPE>
        struct BaseRay
        {
        public:
            Math::BaseVector3<TYPE> origin;
            Math::BaseVector3<TYPE> normal;

        public:
            BaseRay(void)
            {
            }

            BaseRay(const Math::BaseVector3<TYPE> &origin, const Math::BaseVector3<TYPE> &normal)
                : origin(origin)
                , normal(normal)
            {
            }

            BaseRay(const BaseRay<TYPE> &ray)
                : origin(ray.origin)
                , normal(ray.normal)
            {
            }

            BaseRay operator = (const BaseRay<TYPE> &ray)
            {
                origin = ray.origin;
                normal = ray.normal;
                return (*this);
            }

            TYPE getDistance(const BaseOrientedBox<TYPE> &orientedBox) const
            {
                TYPE minimum(TYPE(0));
                TYPE maximum(TYPE(Math::Infinity));
                Math::BaseVector3<TYPE> positionDelta(orientedBox.position - origin);
                Math::BaseMatrix4x4<TYPE> orientedBoxMatrix(orientedBox.rotation);
                Math::BaseVector3<TYPE> orientedBoxHalfSize(orientedBox.size * TYPE(0.5));
                for (UINT32 axis = 0; axis < 3; ++axis)
                {
                    TYPE axisAngle = orientedBoxMatrix.r[axis].xyz.Dot(positionDelta);
                    TYPE rayAngle = normal.Dot(orientedBoxMatrix.r[axis].xyz);
                    if (fabs(rayAngle) > TYPE(Math::Epsilon))
                    {
                        TYPE positionDelta1 = ((axisAngle - orientedBoxHalfSize.xyz[axis]) / rayAngle);
                        TYPE positionDelta2 = ((axisAngle + orientedBoxHalfSize.xyz[axis]) / rayAngle);
                        if (positionDelta1 > positionDelta2)
                        {
                            TYPE positionDeltaSwap = positionDelta1;
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
                            return TYPE(-1);
                        }
                    }
                    else
                    {
                        if ((-axisAngle - orientedBoxHalfSize.xyz[axis]) > TYPE(0) ||
                            (-axisAngle + orientedBoxHalfSize.xyz[axis]) < TYPE(0))
                        {
                            return TYPE(-1);
                        }
                    }
                }

                return minimum;
            }
        };

        typedef BaseRay<float> Ray;
    }; // namespace Shape
}; // namespace Gek

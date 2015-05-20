#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        template <typename TYPE>
        struct BaseOrientedBox
        {
        public:
            Math::BaseMatrix4x4<TYPE> matrix;
            Math::BaseVector3<TYPE> halfsize;

        public:
            BaseOrientedBox(void)
            {
            }

            BaseOrientedBox(const BaseOrientedBox<TYPE> &box)
                : matrix(box.matrix)
                , halfsize(box.halfsize)
            {
            }

            BaseOrientedBox(const BaseAlignedBox<TYPE> &box, const Math::BaseQuaternion<TYPE> &rotation, const Math::BaseVector3<TYPE> &translation)
                : matrix(rotation, (translation + box.getCenter()))
                , halfsize(box.getSize() * 0.5f)
            {
            }

            BaseOrientedBox(const BaseAlignedBox<TYPE> &box, const Math::BaseMatrix4x4<TYPE> &matrix)
                : rotation(matrix, (matrix.translation + box.getCenter()))
                , halfsize(box.getSize() * 0.5f)
            {
            }

            BaseOrientedBox operator = (const BaseOrientedBox<TYPE> &box)
            {
                matrix = box.matrix;
                return (*this);
            }

            int getPosition(const BasePlane<TYPE> &plane) const
            {
                TYPE distance = plane.getDistance(matrix.translation);
                TYPE radiusX = std::abs(matrix.rx.xyz.dot(plane.normal) * halfsize.x);
                TYPE radiusY = std::abs(matrix.ry.xyz.dot(plane.normal) * halfsize.y);
                TYPE radiusZ = std::abs(matrix.rz.xyz.dot(plane.normal) * halfsize.z);
                TYPE radius = (radiusX + radiusY + radiusZ);
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

        typedef BaseOrientedBox<float> OrientedBox;
    }; // namespace Shape
}; // namespace Gek

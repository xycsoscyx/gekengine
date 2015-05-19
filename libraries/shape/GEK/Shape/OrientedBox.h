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
            Math::BaseVector3<TYPE> position;
            Math::BaseMatrix4x4<TYPE> rotation;
            Math::BaseVector3<TYPE> halfsize;

        public:
            BaseOrientedBox(void)
            {
            }

            BaseOrientedBox(const BaseOrientedBox<TYPE> &box)
                : position(box.position)
                , rotation(box.rotation)
                , halfsize(box.halfsize)
            {
            }

            BaseOrientedBox(const BaseAlignedBox<TYPE> &box, const Math::BaseQuaternion<TYPE> &rotation, const Math::BaseVector3<TYPE> &translation)
            {
                rotation = rotation;
                position = (translation + box.getCenter());
                halfsize = (box.getSize() * 0.5f);
            }

            BaseOrientedBox(const BaseAlignedBox<TYPE> &box, const Math::BaseMatrix4x4<TYPE> &matrix)
            {
                rotation = matrix;
                position = (matrix.translation + box.getCenter());
                halfsize = (box.getSize() * 0.5f);
            }

            BaseOrientedBox operator = (const BaseOrientedBox<TYPE> &box)
            {
                position = box.position;
                rotation = box.rotation;
                halfsize = box.halfsize;
                return (*this);
            }

            int getPosition(const BasePlane<TYPE> &plane) const
            {
                TYPE distance = plane.getDistance(position);
                TYPE radiusX = fabs(rotation.rx.xyz.Dot(plane.normal) * halfsize.x);
                TYPE radiusY = fabs(rotation.ry.xyz.Dot(plane.normal) * halfsize.y);
                TYPE radiusZ = fabs(rotation.rz.xyz.Dot(plane.normal) * halfsize.z);
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

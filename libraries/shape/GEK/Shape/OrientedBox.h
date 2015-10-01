#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Shape\Plane.h"
#include "GEK\Shape\AlignedBox.h"

namespace Gek
{
    namespace Shape
    {
        struct OrientedBox
        {
        public:
            Math::Float4x4 matrix;
            Math::Float3 halfsize;

        public:
            OrientedBox(void)
            {
            }

            OrientedBox(const OrientedBox &box)
                : matrix(box.matrix)
                , halfsize(box.halfsize)
            {
            }

            OrientedBox(const AlignedBox &box, const Math::Quaternion &rotation, const Math::Float3 &translation)
                : matrix(rotation, (translation + box.getCenter()))
                , halfsize(box.getSize() * 0.5f)
            {
            }

            OrientedBox(const AlignedBox &box, const Math::Float4x4 &matrix)
                : matrix(matrix, (matrix.translation + box.getCenter()))
                , halfsize(box.getSize() * 0.5f)
            {
            }

            OrientedBox operator = (const OrientedBox &box)
            {
                matrix = box.matrix;
                return (*this);
            }

            int getPosition(const Plane &plane) const
            {
                float distance = plane.getDistance(matrix.translation);
                float radiusX = std::abs(matrix.nx.dot(plane.normal) * halfsize.x);
                float radiusY = std::abs(matrix.ny.dot(plane.normal) * halfsize.y);
                float radiusZ = std::abs(matrix.nz.dot(plane.normal) * halfsize.z);
                float radius = (radiusX + radiusY + radiusZ);
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
    }; // namespace Shape
}; // namespace Gek

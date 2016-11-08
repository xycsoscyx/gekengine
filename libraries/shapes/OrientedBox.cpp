#include "GEK\Math\Convert.hpp"
#include "GEK\Shapes\OrientedBox.hpp"
#include "GEK\Shapes\Plane.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        OrientedBox::OrientedBox(void)
        {
        }

        OrientedBox::OrientedBox(const OrientedBox &box)
            : matrix(box.matrix)
            , halfsize(box.halfsize)
        {
        }

        OrientedBox::OrientedBox(const AlignedBox &box, const Math::Quaternion &rotation, const Math::Float3 &translation)
            : matrix(Math::convert(rotation))
            , halfsize(box.getSize() * 0.5f)
        {
            matrix.translation = (translation + box.getCenter());
        }

        OrientedBox::OrientedBox(const AlignedBox &box, const Math::SIMD::Float4x4 &matrix)
            : matrix(matrix)
            , halfsize(box.getSize() * 0.5f)
        {
            this->matrix.translation += box.getCenter();
        }

        OrientedBox &OrientedBox::operator = (const OrientedBox &box)
        {
            matrix = box.matrix;
            return (*this);
        }

        int OrientedBox::getPosition(const Plane &plane) const
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
    }; // namespace Shapes
}; // namespace Gek

#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Shapes/Plane.hpp"
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

        OrientedBox::OrientedBox(const AlignedBox &box, Math::Quaternion const &rotation, Math::Float3 const &translation)
            : matrix(Math::Float4x4::MakeQuaternionRotation(rotation, translation + box.getCenter()))
            , halfsize(box.getSize() * 0.5f)
        {
        }

        OrientedBox::OrientedBox(const AlignedBox &box, Math::Float4x4 const &matrix)
            : matrix(matrix)
            , halfsize(box.getSize() * 0.5f)
        {
        }

        OrientedBox &OrientedBox::operator = (const OrientedBox &box)
        {
            matrix = box.matrix;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

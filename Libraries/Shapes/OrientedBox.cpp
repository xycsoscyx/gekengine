#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Shapes/Plane.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        OrientedBox::OrientedBox(void) noexcept
        {
        }

        OrientedBox::OrientedBox(const OrientedBox &box) noexcept
            : matrix(box.matrix)
            , halfsize(box.halfsize)
        {
        }

        OrientedBox::OrientedBox(Math::Quaternion const &rotation, Math::Float3 const &translation, const AlignedBox &box) noexcept
            : matrix(Math::Float4x4::MakeQuaternionRotation(rotation, (translation + box.getCenter())))
            , halfsize(box.getHalfSize())
        {
        }

        OrientedBox::OrientedBox(Math::Float4x4 const &matrix, const AlignedBox &box) noexcept
            : matrix(Math::Float4x4::MakeQuaternionRotation(matrix.getRotation(), (matrix.translation.xyz + box.getCenter())))
            , halfsize(box.getHalfSize())
        {
        }

        OrientedBox &OrientedBox::operator = (const OrientedBox &box) noexcept
        {
            matrix = box.matrix;
            return (*this);
        }
    }; // namespace Shapes
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 68c94ed58445f7f7b11fb87263c60bc483158d4d $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 13:54:12 2016 +0000 $
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Shapes/AlignedBox.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct OrientedBox
        {
        public:
            Math::Float3 halfsize;
            Math::Float4x4 matrix;

        public:
            OrientedBox(void) noexcept;
            OrientedBox(OrientedBox const &box) noexcept;
            OrientedBox(Math::Quaternion const &rotation, Math::Float3 const &translation, AlignedBox const &box) noexcept;
            OrientedBox(Math::Float4x4 const &matrix, AlignedBox const &box) noexcept;

            OrientedBox &operator = (OrientedBox const &box) noexcept;
        };
    }; // namespace Shapes
}; // namespace Gek

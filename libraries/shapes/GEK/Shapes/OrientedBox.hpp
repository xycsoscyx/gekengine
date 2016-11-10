/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 68c94ed58445f7f7b11fb87263c60bc483158d4d $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 13:54:12 2016 +0000 $
#pragma once

#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\SIMD4x4.hpp"
#include "GEK\Math\Quaternion.hpp"
#include "GEK\Shapes\AlignedBox.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct Plane;

        struct OrientedBox
        {
        public:
            Math::SIMD::Float4x4 matrix;
            Math::Float3 halfsize;

        public:
            OrientedBox(void);
            OrientedBox(const OrientedBox &box);
            OrientedBox(const AlignedBox &box, const Math::Quaternion &rotation, const Math::Float3 &translation);
            OrientedBox(const AlignedBox &box, const Math::SIMD::Float4x4 &matrix);

            OrientedBox &operator = (const OrientedBox &box);

            int getPosition(const Plane &plane) const;
        };
    }; // namespace Shapes
}; // namespace Gek

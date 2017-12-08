/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector3.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct AlignedBox
        {
        public:
            Math::Float3 minimum;
            Math::Float3 maximum;

        public:
            AlignedBox(void) noexcept;
            AlignedBox(const AlignedBox &box) noexcept;
            AlignedBox(float size) noexcept;
            AlignedBox(Math::Float3 const &minimum, Math::Float3 const &maximum) noexcept;

            AlignedBox &operator = (const AlignedBox &box) noexcept;

            void extend(Math::Float3 &point) noexcept;

            Math::Float3 getSize(void) const noexcept;
            Math::Float3 getHalfSize(void) const noexcept;
            Math::Float3 getCenter(void) const noexcept;
        };
    }; // namespace Shapes
}; // namespace Gek

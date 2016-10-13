/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Float4x4.hpp"
#include "GEK\Shapes\Plane.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct Frustum
        {
        public:
            Plane planes[6];

        public:
            Frustum(const Math::Float4x4 &perspectiveTransform);

            template <class SHAPE>
            bool isVisible(const SHAPE &shape) const
            {
                for (auto &plane : planes)
                {
                    if (shape.getPosition(plane) == -1) return false;
                }

                return true;
            }
        };
    }; // namespace Shapes
}; // namespace Gek

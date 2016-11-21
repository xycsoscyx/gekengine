/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK\Math\Matrix4x4.hpp"
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
            Frustum(void);
            Frustum(const Math::Float4x4 &perspectiveTransform);

            void create(const Math::Float4x4 &perspectiveTransform);

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

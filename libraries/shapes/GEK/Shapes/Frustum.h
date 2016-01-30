#pragma once

#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shapes\Plane.h"

namespace Gek
{
    namespace Shapes
    {
        struct Frustum
        {
        public:
            Plane planes[6];

        public:
            Frustum(const Math::Float4x4 &transform);

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

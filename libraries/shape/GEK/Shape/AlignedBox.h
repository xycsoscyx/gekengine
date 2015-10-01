#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        struct AlignedBox
        {
        public:
            Math::Float3 minimum;
            Math::Float3 maximum;

        public:
            AlignedBox(void)
                : minimum(Math::Infinity)
                , maximum(-Math::Infinity)
            {
            }

            AlignedBox(const AlignedBox &box)
                : minimum(box.minimum)
                , maximum(box.maximum)
            {
            }

            AlignedBox(float size)
                : minimum(-(size * 0.5f))
                , maximum( (size * 0.5f))
            {
            }

            AlignedBox(const Math::Float3 &minimum, const Math::Float3 &maximum)
                : minimum(minimum)
                , maximum(maximum)
            {
            }

            AlignedBox operator = (const AlignedBox &box)
            {
                minimum = box.minimum;
                maximum = box.maximum;
                return (*this);
            }

            void Extend(Math::Float3 &point)
            {
                minimum.x = std::min(point.x, minimum.x);
                minimum.y = std::min(point.y, minimum.y);
                minimum.z = std::min(point.z, minimum.z);

                maximum.x = std::max(point.x, maximum.x);
                maximum.y = std::max(point.y, maximum.y);
                maximum.z = std::max(point.z, maximum.z);
            }

            Math::Float3 getSize(void) const
            {
                return (maximum - minimum);
            }

            Math::Float3 getCenter(void) const
            {
                return (minimum + (getSize() * 0.5f));
            }

            int getPosition(const Plane &plane) const
            {
                if (plane.getDistance(Math::Float3((plane.normal.x > 0.0f ? maximum.x : minimum.x),
                                                   (plane.normal.y > 0.0f ? maximum.y : minimum.y),
                                                   (plane.normal.z > 0.0f ? maximum.z : minimum.z))) < 0.0f)
                {
                    return -1;
                }

                if (plane.getDistance(Math::Float3((plane.normal.x < 0.0f ? maximum.x : minimum.x),
                                                   (plane.normal.y < 0.0f ? maximum.y : minimum.y),
                                                   (plane.normal.z < 0.0f ? maximum.z : minimum.z))) < 0.0f)
                {
                    return 0;
                }

                return 1;
            }
        };
    }; // namespace Shape
}; // namespace Gek

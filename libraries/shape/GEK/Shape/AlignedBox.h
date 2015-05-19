#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Shape\Plane.h"

namespace Gek
{
    namespace Shape
    {
        template <typename TYPE>
        struct BaseAlignedBox
        {
        public:
            Math::BaseVector3<TYPE> minimum;
            Math::BaseVector3<TYPE> maximum;

        public:
            BaseAlignedBox(void)
                : minimum(Math::Infinity)
                , maximum(-Math::Infinity)
            {
            }

            BaseAlignedBox(const BaseAlignedBox<TYPE> &box)
                : minimum(box.minimum)
                , maximum(box.maximum)
            {
            }

            BaseAlignedBox(TYPE size)
                : minimum(-(size * 0.5f))
                , maximum( (size * 0.5f))
            {
            }

            BaseAlignedBox(const Math::BaseVector3<TYPE> &minimum, const Math::BaseVector3<TYPE> &maximum)
                : minimum(minimum)
                , maximum(maximum)
            {
            }

            BaseAlignedBox operator = (const BaseAlignedBox<TYPE> &box)
            {
                minimum = box.minimum;
                maximum = box.maximum;
                return (*this);
            }

            void Extend(Math::BaseVector3<TYPE> &point)
            {
                minimum.x = min(point.x, minimum.x);
                minimum.y = min(point.y, minimum.y);
                minimum.z = min(point.z, minimum.z);

                maximum.x = max(point.x, maximum.x);
                maximum.y = max(point.y, maximum.y);
                maximum.z = max(point.z, maximum.z);
            }

            Math::BaseVector3<TYPE> getSize(void) const
            {
                return (maximum - minimum);
            }

            Math::BaseVector3<TYPE> getCenter(void) const
            {
                return (minimum + (getSize() * 0.5f));
            }

            int getPosition(const BasePlane<TYPE> &plane) const
            {
                if (plane.Distance(Math::BaseVector3<TYPE>((plane.normal.x > 0.0f ? maximum.x : minimum.x),
                                                           (plane.normal.y > 0.0f ? maximum.y : minimum.y),
                                                           (plane.normal.z > 0.0f ? maximum.z : minimum.z))) < 0.0f)
                {
                    return -1;
                }

                if (plane.Distance(Math::BaseVector3<TYPE>((plane.normal.x < 0.0f ? maximum.x : minimum.x),
                                                           (plane.normal.y < 0.0f ? maximum.y : minimum.y),
                                                           (plane.normal.z < 0.0f ? maximum.z : minimum.z))) < 0.0f)
                {
                    return 0;
                }

                return 1;
            }
        };
        
        typedef BaseAlignedBox<float> AlignedBox;
    }; // namespace Shape
}; // namespace Gek

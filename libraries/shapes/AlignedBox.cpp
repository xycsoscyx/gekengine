#include "GEK\Shapes\AlignedBox.hpp"
#include "GEK\Shapes\Plane.hpp"
#include "GEK\Math\Constants.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        AlignedBox::AlignedBox(void)
            : minimum(Math::Infinity)
            , maximum(Math::NegativeInfinity)
        {
        }

        AlignedBox::AlignedBox(const AlignedBox &box)
            : minimum(box.minimum)
            , maximum(box.maximum)
        {
        }

        AlignedBox::AlignedBox(float size)
            : minimum(-(size * 0.5f))
            , maximum((size * 0.5f))
        {
        }

        AlignedBox::AlignedBox(const Math::Float3 &minimum, const Math::Float3 &maximum)
            : minimum(minimum)
            , maximum(maximum)
        {
        }

        AlignedBox &AlignedBox::operator = (const AlignedBox &box)
        {
            minimum = box.minimum;
            maximum = box.maximum;
            return (*this);
        }

        void AlignedBox::extend(Math::Float3 &point)
        {
            minimum.x = std::min(point.x, minimum.x);
            minimum.y = std::min(point.y, minimum.y);
            minimum.z = std::min(point.z, minimum.z);

            maximum.x = std::max(point.x, maximum.x);
            maximum.y = std::max(point.y, maximum.y);
            maximum.z = std::max(point.z, maximum.z);
        }

        Math::Float3 AlignedBox::getSize(void) const
        {
            return (maximum - minimum);
        }

        Math::Float3 AlignedBox::getCenter(void) const
        {
            return (minimum + (getSize() * 0.5f));
        }

        int AlignedBox::getPosition(const Plane &plane) const
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
    }; // namespace Shapes
}; // namespace Gek

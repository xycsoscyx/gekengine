#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Shapes/Plane.hpp"
#include "GEK/Math/Common.hpp"
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

        AlignedBox::AlignedBox(Math::Float3 const &minimum, Math::Float3 const &maximum)
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
            minimum = minimum.getMinimum(point);
            maximum = maximum.getMaximum(point);
        }

        Math::Float3 AlignedBox::getSize(void) const
        {
            return (maximum - minimum);
        }

        Math::Float3 AlignedBox::getCenter(void) const
        {
            return (minimum + (getSize() * 0.5f));
        }
    }; // namespace Shapes
}; // namespace Gek

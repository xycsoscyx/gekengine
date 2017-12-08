#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Shapes/Plane.hpp"
#include "GEK/Math/Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Shapes
    {
        AlignedBox::AlignedBox(void) noexcept
            : minimum(Math::Infinity)
            , maximum(Math::NegativeInfinity)
        {
        }

        AlignedBox::AlignedBox(const AlignedBox &box) noexcept
            : minimum(box.minimum)
            , maximum(box.maximum)
        {
        }

        AlignedBox::AlignedBox(float size) noexcept
            : minimum(-(size * 0.5f))
            , maximum((size * 0.5f))
        {
        }

        AlignedBox::AlignedBox(Math::Float3 const &minimum, Math::Float3 const &maximum) noexcept
            : minimum(minimum)
            , maximum(maximum)
        {
        }

        AlignedBox &AlignedBox::operator = (const AlignedBox &box) noexcept
        {
            minimum = box.minimum;
            maximum = box.maximum;
            return (*this);
        }

        void AlignedBox::extend(Math::Float3 &point) noexcept
        {
            minimum = minimum.getMinimum(point);
            maximum = maximum.getMaximum(point);
        }

        Math::Float3 AlignedBox::getSize(void) const noexcept
        {
            return (maximum - minimum);
        }

        Math::Float3 AlignedBox::getHalfSize(void) const noexcept
        {
            return ((maximum - minimum) * 0.5f);
        }

        Math::Float3 AlignedBox::getCenter(void) const noexcept
        {
            return (minimum + getHalfSize());
        }
    }; // namespace Shapes
}; // namespace Gek

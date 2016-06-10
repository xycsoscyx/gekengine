#include "GEK\Math\Float2.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Float2 Float2::Zero(0.0f, 0.0f);
        const Float2 Float2::One(1.0f, 1.0f);

        float Float2::getLengthSquared(void) const
        {
            return ((x * x) + (y * y));
        }

        float Float2::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float2::getDistance(const Float2 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float2 Float2::getNormal(void) const
        {
            return ((*this) / getLength());
        }

        void Float2::normalize(void)
        {
            (*this) = getNormal();
        }

        float Float2::dot(const Float2 &vector) const
        {
            return ((x * vector.x) + (y * vector.y));
        }

        Float2 Float2::lerp(const Float2 &vector, float factor) const
        {
            return Math::lerp((*this), vector, factor);
        }

        bool Float2::operator < (const Float2 &vector) const
        {
            if (x >= vector.x) return false;
            if (y >= vector.y) return false;
            return true;
        }

        bool Float2::operator > (const Float2 &vector) const
        {
            if (x <= vector.x) return false;
            if (y <= vector.y) return false;
            return true;
        }

        bool Float2::operator <= (const Float2 &vector) const
        {
            if (x > vector.x) return false;
            if (y > vector.y) return false;
            return true;
        }

        bool Float2::operator >= (const Float2 &vector) const
        {
            if (x < vector.x) return false;
            if (y < vector.y) return false;
            return true;
        }

        bool Float2::operator == (const Float2 &vector) const
        {
            if (x != vector.x) return false;
            if (y != vector.y) return false;
            return true;
        }

        bool Float2::operator != (const Float2 &vector) const
        {
            if (x != vector.x) return true;
            if (y != vector.y) return true;
            return false;
        }
    }; // namespace Math
}; // namespace Gek

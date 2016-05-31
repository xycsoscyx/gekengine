#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float4 Float3::w(float w) const
        {
            return Float4(x, y, z, w);
        }

        float Float3::getLengthSquared(void) const
        {
            return ((x * x) + (y * y) + (z * z));
        }

        float Float3::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float3::getDistance(const Float3 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float3 Float3::getNormal(void) const
        {
            return ((*this) / getLength());
        }

        float Float3::dot(const Float3 &vector) const
        {
            return ((x * vector.x) + (y * vector.y) + (z * vector.z));
        }

        Float3 Float3::cross(const Float3 &vector) const
        {
            return Float3(((y * vector.z) - (z * vector.y)),
                          ((z * vector.x) - (x * vector.z)),
                          ((x * vector.y) - (y * vector.x)));
        }

        Float3 Float3::lerp(const Float3 &vector, float factor) const
        {
            return Math::lerp((*this), vector, factor);
        }

        void Float3::normalize(void)
        {
            (*this) = getNormal();
        }

        bool Float3::operator < (const Float3 &vector) const
        {
            if (x >= vector.x) return false;
            if (y >= vector.y) return false;
            if (z >= vector.z) return false;
            return true;
        }

        bool Float3::operator > (const Float3 &vector) const
        {
            if (x <= vector.x) return false;
            if (y <= vector.y) return false;
            if (z <= vector.z) return false;
            return true;
        }

        bool Float3::operator <= (const Float3 &vector) const
        {
            if (x > vector.x) return false;
            if (y > vector.y) return false;
            if (z > vector.z) return false;
            return true;
        }

        bool Float3::operator >= (const Float3 &vector) const
        {
            if (x < vector.x) return false;
            if (y < vector.y) return false;
            if (z < vector.z) return false;
            return true;
        }

        bool Float3::operator == (const Float3 &vector) const
        {
            if (x != vector.x) return false;
            if (y != vector.y) return false;
            if (z != vector.z) return false;
            return true;
        }

        bool Float3::operator != (const Float3 &vector) const
        {
            if (x != vector.x) return true;
            if (y != vector.y) return true;
            if (z != vector.z) return true;
            return false;
        }
    }; // namespace Math
}; // namespace Gek

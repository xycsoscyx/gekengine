#include "GEK\Math\Float4.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Float4 Float4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
        const Float4 Float4::One(1.0f, 1.0f, 1.0f, 1.0f);
		const Float4 Float4::Identity(0.0f, 0.0f, 0.0f, 1.0f);

        Float3 Float4::getXYZ(void) const
        {
            return Float3(x, y, z);
        }

        float Float4::getLengthSquared(void) const
        {
            return this->dot(*this);
        }

        float Float4::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float4::getDistance(const Float4 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float4 Float4::getNormal(void) const
        {
            return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
        }

        float Float4::dot(const Float4 &vector) const
        {
            return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
        }

        Float4 Float4::lerp(const Float4 &vector, float factor) const
        {
            return Math::lerp((*this), vector, factor);
        }

        void Float4::normalize(void)
        {
            (*this) = getNormal();
        }

        bool Float4::operator < (const Float4 &vector) const
        {
            if (x >= vector.x) return false;
            if (y >= vector.y) return false;
            if (z >= vector.z) return false;
            if (w >= vector.w) return false;
            return true;
        }

        bool Float4::operator > (const Float4 &vector) const
        {
            if (x <= vector.x) return false;
            if (y <= vector.y) return false;
            if (z <= vector.z) return false;
            if (w <= vector.w) return false;
            return true;
        }

        bool Float4::operator <= (const Float4 &vector) const
        {
            if (x > vector.x) return false;
            if (y > vector.y) return false;
            if (z > vector.z) return false;
            if (w > vector.w) return false;
            return true;
        }

        bool Float4::operator >= (const Float4 &vector) const
        {
            if (x < vector.x) return false;
            if (y < vector.y) return false;
            if (z < vector.z) return false;
            if (w < vector.w) return false;
            return true;
        }

        bool Float4::operator == (const Float4 &vector) const
        {
            if (x != vector.x) return false;
            if (y != vector.y) return false;
            if (z != vector.z) return false;
            if (w != vector.w) return false;
            return true;
        }

        bool Float4::operator != (const Float4 &vector) const
        {
            if (x != vector.x) return true;
            if (y != vector.y) return true;
            if (z != vector.z) return true;
            if (w != vector.w) return true;
            return false;
        }
    }; // namespace Math
}; // namespace Gek

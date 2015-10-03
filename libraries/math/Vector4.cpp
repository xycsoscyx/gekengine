#include "GEK\Math\Vector4.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float3 Float4::getXYZ(void) const
        {
            return Float3(x, y, z);
        }

        void Float4::set(float value)
        {
            simd = _mm_set1_ps(value);
        }

        void Float4::set(float x, float y, float z, float w)
        {
            simd = _mm_setr_ps(x, y, z, w);
        }

        void Float4::set(const Float4 &vector)
        {
            simd = vector.simd;
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
            return ((*this) / getLength());
        }

        float Float4::dot(const Float4 &vector) const
        {
            return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
        }

        Float4 Float4::lerp(const Float4 &vector, float factor) const
        {
            return Gek::Math::lerp((*this), vector, factor);
        }

        void Float4::normalize(void)
        {
            (*this).set(getNormal());
        }

        float Float4::operator [] (int axis) const
        {
            return data[axis];
        }

        float &Float4::operator [] (int axis)
        {
            return data[axis];
        }

        Float4::operator const float *() const
        {
            return data;
        }

        Float4::operator float *()
        {
            return data;
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

        Float4 Float4::operator = (const Float4 &vector)
        {
            x = vector.x;
            y = vector.y;
            z = vector.z;
            w = vector.w;
            return (*this);
        }

        void Float4::operator -= (const Float4 &vector)
        {
            x -= vector.x;
            y -= vector.y;
            z -= vector.z;
            w -= vector.w;
        }

        void Float4::operator += (const Float4 &vector)
        {
            x += vector.x;
            y += vector.y;
            z += vector.z;
            w += vector.w;
        }

        void Float4::operator /= (const Float4 &vector)
        {
            x /= vector.x;
            y /= vector.y;
            z /= vector.z;
            w /= vector.w;
        }

        void Float4::operator *= (const Float4 &vector)
        {
            x *= vector.x;
            y *= vector.y;
            z *= vector.z;
            w *= vector.w;
        }

        Float4 Float4::operator - (const Float4 &vector) const
        {
            return Float4((x - vector.x), (y - vector.y), (z - vector.z), (w - vector.w));
        }

        Float4 Float4::operator + (const Float4 &vector) const
        {
            return Float4((x + vector.x), (y + vector.y), (z + vector.z), (w + vector.w));
        }

        Float4 Float4::operator / (const Float4 &vector) const
        {
            return Float4((x / vector.x), (y / vector.y), (z / vector.z), (w / vector.w));
        }

        Float4 Float4::operator * (const Float4 &vector) const
        {
            return Float4((x * vector.x), (y * vector.y), (z * vector.z), (w * vector.w));
        }

        Float4 operator - (const Float4 &vector)
        {
            return Float4(-vector.x, -vector.y, -vector.z, -vector.w);
        }
    }; // namespace Math
}; // namespace Gek

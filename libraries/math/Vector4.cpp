#include "GEK\Math\Vector4.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float4::Float4(void)
            : data{ 0.0f, 0.0f, 0.0f, 0.0f }
        {
        }

        Float4::Float4(float value)
            : data{ value, value, value, value }
        {
        }

        Float4::Float4(__m128 simd)
            : simd(simd)
        {
        }

        Float4::Float4(const float(&data)[4])
            : data{ data[0], data[1], data[2], data[3] }
        {
        }

        Float4::Float4(const float *data)
            : data{ data[0], data[1], data[2], data[3] }
        {
        }

        Float4::Float4(const Float4 &vector)
            : simd(vector.simd)
        {
        }

        Float4::Float4(float x, float y, float z, float w)
            : x(x)
            , y(y)
            , z(z)
            , w(w)
        {
        }

        Float3 Float4::getXYZ(void) const
        {
            return Float3(x, y, z);
        }

        void Float4::set(float value)
        {
            this->x = this->y = this->z = this->w = value;
        }

        void Float4::set(float x, float y, float z, float w)
        {
            this->x = x;
            this->y = y;
            this->z = z;
            this->w = w;
        }

        void Float4::setLength(float length)
        {
            (*this) *= (length / getLength());
        }

        float Float4::getLengthSquared(void) const
        {
            return ((x * x) + (y * y) + (z * z) + (w * w));
        }

        float Float4::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float4::getMax(void) const
        {
            return std::max(std::max(std::max(x, y), z), w);
        }

        float Float4::getDistance(const Float4 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float4 Float4::getNormal(void) const
        {
            float length(getLength());
            if (length != 0.0f)
            {
                return ((*this) * (1.0f / length));
            }

            return (*this);
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
            (*this) = getNormal();
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

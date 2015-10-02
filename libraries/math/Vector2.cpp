#include "GEK\Math\Vector2.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float2::Float2(void)
            : data{ 0.0f, 0.0f }
        {
        }

        Float2::Float2(const float(&data)[2])
            : data{ data[0], data[1] }
        {
        }

        Float2::Float2(const float *data)
            : data{ data[0], data[1] }
        {
        }

        Float2::Float2(float scalar)
            : data{ scalar, scalar }
        {
            x = y = scalar;
        }

        Float2::Float2(const Float2 &vector)
            : x(vector.x)
            , y(vector.y)
        {
        }

        Float2::Float2(float x, float y)
            : x(x)
            , y(y)
        {
        }

        void Float2::set(float value)
        {
            this->x = this->y = value;
        }

        void Float2::set(float x, float y)
        {
            this->x = x;
            this->y = y;
        }

        void Float2::setLength(float length)
        {
            (*this) *= (length / getLength());
        }

        float Float2::getLengthSquared(void) const
        {
            return ((x * x) + (y * y));
        }

        float Float2::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float2::getMax(void) const
        {
            return std::max(x, y);
        }

        float Float2::getDistance(const Float2 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float2 Float2::getNormal(void) const
        {
            float length = getLength();
            if (length != 0.0f)
            {
                return ((*this) * (1.0f / length));
            }

            return (*this);
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
            return Gek::Math::lerp((*this), vector, factor);
        }

        float Float2::operator [] (int axis) const
        {
            return data[axis];
        }

        float &Float2::operator [] (int axis)
        {
            return data[axis];
        }

        Float2::operator const float *() const
        {
            return data;
        }

        Float2::operator float *()
        {
            return data;
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

        Float2 Float2::operator = (const Float2 &vector)
        {
            x = vector.x;
            y = vector.y;
            return (*this);
        }

        void Float2::operator -= (const Float2 &vector)
        {
            x -= vector.x;
            y -= vector.y;
        }

        void Float2::operator += (const Float2 &vector)
        {
            x += vector.x;
            y += vector.y;
        }

        void Float2::operator /= (const Float2 &vector)
        {
            x /= vector.x;
            y /= vector.y;
        }

        void Float2::operator *= (const Float2 &vector)
        {
            x *= vector.x;
            y *= vector.y;
        }

        Float2 Float2::operator - (const Float2 &vector) const
        {
            return Float2((x - vector.x), (y - vector.y));
        }

        Float2 Float2::operator + (const Float2 &vector) const
        {
            return Float2((x + vector.x), (y + vector.y));
        }

        Float2 Float2::operator / (const Float2 &vector) const
        {
            return Float2((x / vector.x), (y / vector.y));
        }

        Float2 Float2::operator * (const Float2 &vector) const
        {
            return Float2((x * vector.x), (y * vector.y));
        }

        Float2 operator - (const Float2 &vector)
        {
            return Float2(-vector.x, -vector.y);
        }
    }; // namespace Math
}; // namespace Gek

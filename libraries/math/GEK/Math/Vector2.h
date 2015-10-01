#pragma once

#include <algorithm>
#include "Gek\Math\Common.h"

namespace Gek
{
    namespace Math
    {
        struct Float2
        {
        public:
            union
            {
                struct { float x, y; };
                struct { float u, v; };
                struct { float data[2]; };
            };

        public:
            Float2(void)
                : data{ 0.0f, 0.0f }
            {
            }

            Float2(const float(&data)[2])
                : data { data[0], data[1] }
            {
            }

            Float2(const float *data)
                : data{ data[0], data[1] }
            {
            }

            Float2(float scalar)
                : data{ scalar, scalar }
            {
                x = y = scalar;
            }

            Float2(const Float2 &vector)
                : x(vector.x)
                , y(vector.y)
            {
            }

            Float2(float x, float y)
                : x(x)
                , y(y)
            {
            }

            void set(float x, float y)
            {
                this->x = x;
                this->y = y;
            }

            void setLength(float length)
            {
                (*this) *= (length / getLength());
            }

            float getLengthSquared(void) const
            {
                return ((x * x) + (y * y));
            }

            float getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            float getMax(void) const
            {
                return std::max(x, y);
            }

            float getDistance(const Float2 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Float2 getNormal(void) const
            {
                float length = getLength();
                if (length != 0.0f)
                {
                    return ((*this) * (1.0f / length));
                }

                return (*this);
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            float dot(const Float2 &vector) const
            {
                return ((x * vector.x) + (y * vector.y));
            }

            Float2 lerp(const Float2 &vector, float factor) const
            {
                return Gek::Math::lerp((*this), vector, factor);
            }

            float operator [] (int axis) const
            {
                return data[axis];
            }

            float &operator [] (int axis)
            {
                return data[axis];
            }

            operator const float *() const
            {
                return data;
            }

            operator float *()
            {
                return data;
            }

            bool operator < (const Float2 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                return true;
            }

            bool operator > (const Float2 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                return true;
            }

            bool operator <= (const Float2 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                return true;
            }

            bool operator >= (const Float2 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                return true;
            }

            bool operator == (const Float2 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                return true;
            }

            bool operator != (const Float2 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                return false;
            }

            Float2 operator = (float scalar)
            {
                x = y = scalar;
                return (*this);
            }

            Float2 operator = (const Float2 &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            void operator -= (const Float2 &vector)
            {
                x -= vector.x;
                y -= vector.y;
            }

            void operator += (const Float2 &vector)
            {
                x += vector.x;
                y += vector.y;
            }

            void operator /= (const Float2 &vector)
            {
                x /= vector.x;
                y /= vector.y;
            }

            void operator *= (const Float2 &vector)
            {
                x *= vector.x;
                y *= vector.y;
            }

            void operator -= (float scalar)
            {
                x -= scalar;
                y -= scalar;
            }

            void operator += (float scalar)
            {
                x += scalar;
                y += scalar;
            }

            void operator /= (float scalar)
            {
                x /= scalar;
                y /= scalar;
            }

            void operator *= (float scalar)
            {
                x *= scalar;
                y *= scalar;
            }

            Float2 operator - (const Float2 &vector) const
            {
                return Float2((x - vector.x), (y - vector.y));
            }

            Float2 operator + (const Float2 &vector) const
            {
                return Float2((x + vector.x), (y + vector.y));
            }

            Float2 operator / (const Float2 &vector) const
            {
                return Float2((x / vector.x), (y / vector.y));
            }

            Float2 operator * (const Float2 &vector) const
            {
                return Float2((x * vector.x), (y * vector.y));
            }

            Float2 operator - (float scalar) const
            {
                return Float2((x - scalar), (y - scalar));
            }

            Float2 operator + (float scalar) const
            {
                return Float2((x + scalar), (y + scalar));
            }

            Float2 operator / (float scalar) const
            {
                return Float2((x / scalar), (y / scalar));
            }

            Float2 operator * (float scalar) const
            {
                return Float2((x * scalar), (y * scalar));
            }
        };

        Float2 operator - (const Float2 &vector)
        {
            return Float2(-vector.x, -vector.y);
        }

        Float2 operator - (float scalar, const Float2 &vector)
        {
            return Float2((scalar - vector.x), (scalar - vector.y));
        }

        Float2 operator + (float scalar, const Float2 &vector)
        {
            return Float2((scalar + vector.x), (scalar + vector.y));
        }

        Float2 operator / (float scalar, const Float2 &vector)
        {
            return Float2((scalar / vector.x), (scalar / vector.y));
        }

        Float2 operator * (float scalar, const Float2 &vector)
        {
            return Float2((scalar * vector.x), (scalar * vector.y));
        }
    }; // namespace Math
}; // namespace Gek

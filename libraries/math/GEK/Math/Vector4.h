#pragma once

#include <algorithm>
#include <xmmintrin.h>
#include "Gek\Math\Common.h"
#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"

namespace Gek
{
    namespace Math
    {
        struct Float4
        {
        public:
            union
            {
                struct { float x, y, z, w; };
                struct { float r, g, b, a; };
                struct { float data[4]; };
                struct { __m128 simd; };
            };

        public:
            Float4(void)
                : data{ 0.0f, 0.0f, 0.0f, 0.0f }
            {
            }

            Float4(float scalar)
                : data{ scalar, scalar, scalar, scalar }
            {
            }

            Float4(__m128 simd)
                : simd(simd)
            {
            }

            Float4(const float (&data)[4])
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            Float4(const float *data)
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            Float4(const Float4 &vector)
                : simd(vector.simd)
            {
            }

            Float4(float x, float y, float z, float w)
                : x(x)
                , y(y)
                , z(z)
                , w(w)
            {
            }

            void set(float x, float y, float z, float w)
            {
                this->x = x;
                this->y = y;
                this->z = z;
                this->w = w;
            }

            void setLength(float length)
            {
                (*this) *= (length / getLength());
            }

            float getLengthSquared(void) const
            {
                return ((x * x) + (y * y) + (z * z) + (w * w));
            }

            float getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            float getMax(void) const
            {
                return std::max(std::max(std::max(x, y), z), w);
            }

            float getDistance(const Float4 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Float4 getNormal(void) const
            {
                float length(getLength());
                if (length != 0.0f)
                {
                    return ((*this) * (1.0f / length));
                }

                return (*this);
            }

            float dot(const Float4 &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
            }

            Float4 lerp(const Float4 &vector, float factor) const
            {
                return Gek::Math::lerp((*this), vector, factor);
            }

            void normalize(void)
            {
                (*this) = getNormal();
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

            bool operator < (const Float4 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                if (w >= vector.w) return false;
                return true;
            }

            bool operator > (const Float4 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                if (w <= vector.w) return false;
                return true;
            }

            bool operator <= (const Float4 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                if (w > vector.w) return false;
                return true;
            }

            bool operator >= (const Float4 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                if (w < vector.w) return false;
                return true;
            }

            bool operator == (const Float4 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                if (w != vector.w) return false;
                return true;
            }

            bool operator != (const Float4 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                if (w != vector.w) return true;
                return false;
            }

            Float4 operator = (float scalar)
            {
                x = y = z = w = scalar;
                return (*this);
            }

            Float4 operator = (const Float4 &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = vector.w;
                return (*this);
            }

            void operator -= (const Float4 &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
                w -= vector.w;
            }

            void operator += (const Float4 &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
                w += vector.w;
            }

            void operator /= (const Float4 &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
                w /= vector.w;
            }

            void operator *= (const Float4 &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
                w *= vector.w;
            }

            void operator -= (float scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
                w -= scalar;
            }

            void operator += (float scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
                w += scalar;
            }

            void operator /= (float scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
                w /= scalar;
            }

            void operator *= (float scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                w *= scalar;
            }

            Float4 operator - (const Float4 &vector) const
            {
                return Float4((x - vector.x), (y - vector.y), (z - vector.z), (w - vector.w));
            }

            Float4 operator + (const Float4 &vector) const
            {
                return Float4((x + vector.x), (y + vector.y), (z + vector.z), (w + vector.w));
            }

            Float4 operator / (const Float4 &vector) const
            {
                return Float4((x / vector.x), (y / vector.y), (z / vector.z), (w / vector.w));
            }

            Float4 operator * (const Float4 &vector) const
            {
                return Float4((x * vector.x), (y * vector.y), (z * vector.z), (w * vector.w));
            }

            Float4 operator - (float scalar) const
            {
                return Float4((x - scalar), (y - scalar), (z - scalar), (w - scalar));
            }

            Float4 operator + (float scalar) const
            {
                return Float4((x + scalar), (y + scalar), (z + scalar), (w + scalar));
            }

            Float4 operator / (float scalar) const
            {
                return Float4((x / scalar), (y / scalar), (z / scalar), (w / scalar));
            }

            Float4 operator * (float scalar) const
            {
                return Float4((x * scalar), (y * scalar), (z * scalar), (w * scalar));
            }
        };

        Float4 operator - (const Float4 &vector)
        {
            return Float4(-vector.x, -vector.y, -vector.z, -vector.w);
        }

        Float4 operator - (float scalar, const Float4 &vector)
        {
            return Float4((scalar - vector.x), (scalar - vector.y), (scalar - vector.z), (scalar - vector.w));
        }

        Float4 operator + (float scalar, const Float4 &vector)
        {
            return Float4((scalar + vector.x), (scalar + vector.y), (scalar + vector.z), (scalar + vector.w));
        }

        Float4 operator / (float scalar, const Float4 &vector)
        {
            return Float4((scalar / vector.x), (scalar / vector.y), (scalar / vector.z), (scalar / vector.w));
        }

        Float4 operator * (float scalar, const Float4 &vector)
        {
            return Float4((scalar * vector.x), (scalar * vector.y), (scalar * vector.z), (scalar * vector.w));
        }
    }; // namespace Math
}; // namespace Gek

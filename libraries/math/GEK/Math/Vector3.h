#pragma once

#include <algorithm>
#include "Gek\Math\Common.h"

namespace Gek
{
    namespace Math
    {
        struct Float3
        {
        public:
            union
            {
                struct { float x, y, z; };
                struct { float r, g, b; };
                struct { float u, v, w; };
                struct { float data[3]; };
            };

        public:
            Float3(void)
                : data{ 0.0f, 0.0f, 0.0f }
            {
            }

            Float3(const float (&data)[3])
                : data{ data[0], data[1], data[2] }
            {
            }

            Float3(const float *data)
                : data{ data[0], data[1], data[2] }
            {
            }

            Float3(float scalar)
                : data{ scalar, scalar, scalar }
            {
            }

            Float3(const Float3 &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
            {
            }

            Float3(float x, float y, float z)
                : x(x)
                , y(y)
                , z(z)
            {
            }

            void set(float x, float y, float z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            void setLength(float length)
            {
                (*this) *= (length / getLength());
            }

            float getLengthSquared(void) const
            {
                return ((x * x) + (y * y) + (z * z));
            }

            float getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            float getMax(void) const
            {
                return std::max(std::max(x, y), z);
            }

            float getDistance(const Float3 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Float3 getNormal(void) const
            {
                float length(getLength());
                if (length != 0.0f)
                {
                    return ((*this) * (1.0f / length));
                }

                return (*this);
            }

            float dot(const Float3 &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z));
            }

            Float3 cross(const Float3 &vector) const
            {
                return Float3(((y * vector.z) - (z * vector.y)),
                    ((z * vector.x) - (x * vector.z)),
                    ((x * vector.y) - (y * vector.x)));
            }

            Float3 lerp(const Float3 &vector, float factor) const
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

            bool operator < (const Float3 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                return true;
            }

            bool operator > (const Float3 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                return true;
            }

            bool operator <= (const Float3 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                return true;
            }

            bool operator >= (const Float3 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                return true;
            }

            bool operator == (const Float3 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                return true;
            }

            bool operator != (const Float3 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                return false;
            }

            Float3 operator = (float scalar)
            {
                x = y = z = scalar;
                return (*this);
            }

            Float3 operator = (const Float3 &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            void operator -= (const Float3 &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            void operator += (const Float3 &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            void operator /= (const Float3 &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            void operator *= (const Float3 &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            void operator -= (float scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            void operator += (float scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            void operator /= (float scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            void operator *= (float scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            Float3 operator - (const Float3 &vector) const
            {
                return Float3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            Float3 operator + (const Float3 &vector) const
            {
                return Float3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            Float3 operator / (const Float3 &vector) const
            {
                return Float3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            Float3 operator * (const Float3 &vector) const
            {
                return Float3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            Float3 operator - (float scalar) const
            {
                return Float3((x - scalar), (y - scalar), (z - scalar));
            }

            Float3 operator + (float scalar) const
            {
                return Float3((x + scalar), (y + scalar), (z + scalar));
            }

            Float3 operator / (float scalar) const
            {
                return Float3((x / scalar), (y / scalar), (z / scalar));
            }

            Float3 operator * (float scalar) const
            {
                return Float3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        Float3 operator - (const Float3 &vector)
        {
            return Float3(-vector.x, -vector.y, -vector.z);
        }

        Float3 operator - (float scalar, const Float3 &vector)
        {
            return Float3((scalar - vector.x), (scalar - vector.y), (scalar - vector.z));
        }

        Float3 operator + (float scalar, const Float3 &vector)
        {
            return Float3((scalar + vector.x), (scalar + vector.y), (scalar + vector.z));
        }

        Float3 operator / (float scalar, const Float3 &vector)
        {
            return Float3((scalar / vector.x), (scalar / vector.y), (scalar / vector.z));
        }

        Float3 operator * (float scalar, const Float3 &vector)
        {
            return Float3((scalar * vector.x), (scalar * vector.y), (scalar * vector.z));
        }
    }; // namespace Math
}; // namespace Gek

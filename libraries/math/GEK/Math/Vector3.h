#pragma once

#include <cmath>
#include <initializer_list>
#include "GEK\Math\Vector2.h"

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector2;
        template <typename TYPE> struct BaseVector4;

        template <typename TYPE>
        struct BaseVector3
        {
        public:
            union
            {
                struct { TYPE x, y, z; };
                struct { TYPE r, g, b; };
                struct { TYPE xyz[3]; };
                struct { TYPE rgb[3]; };
                struct { TYPE data[3]; };
            };

        public:
            BaseVector3(void)
                : x(0)
                , y(0)
                , z(0)
            {
            }

            BaseVector3(const std::initializer_list<float> &list)
            {
                memcpy(this->data, list.begin(), sizeof(this->data));
            }

            BaseVector3(const TYPE *vector)
            {
                memcpy(this->data, vector, sizeof(this->data));
            }

            BaseVector3(TYPE scalar)
                : x(scalar)
                , y(scalar)
                , z(scalar)
            {
            }

            BaseVector3(const BaseVector2<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(0)
            {
            }

            BaseVector3(const BaseVector3<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
            {
            }

            BaseVector3(const BaseVector4<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
            {
            }

            BaseVector3(TYPE x, TYPE y, TYPE z)
                : x(x)
                , y(y)
                , z(z)
            {
            }

            void set(TYPE x, TYPE y, TYPE z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            void setLength(TYPE length)
            {
                (*this) *= (length / getLength());
            }

            TYPE getLengthSquared(void) const
            {
                return ((x * x) + (y * y) + (z * z));
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            TYPE getMax(void) const
            {
                return std::max(max(x, y), z);
            }

            TYPE getDistance(const BaseVector4<TYPE> &vector) const
            {
                return (vector - (*this)).getLength();
            }

            BaseVector3 getNormal(void) const
            {
                TYPE length(getLength());
                if (length != 0.0f)
                {
                    return ((*this) * (1.0f / length));
                }

                return (*this);
            }

            TYPE dot(const BaseVector4<TYPE> &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z));
            }

            BaseVector3 cross(const BaseVector4<TYPE> &vector) const
            {
                return BaseVector3(((y * vector.z) - (z * vector.y)),
                    ((z * vector.x) - (x * vector.z)),
                    ((x * vector.y) - (y * vector.x)));
            }

            BaseVector3 lerp(const BaseVector4<TYPE> &vector, TYPE factor) const
            {
                return lerp((*this), vector, factor);
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            TYPE operator [] (int axis) const
            {
                return xyz[axis];
            }

            TYPE &operator [] (int axis)
            {
                return xyz[axis];
            }

            bool operator < (const BaseVector4<TYPE> &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                return true;
            }

            bool operator > (const BaseVector4<TYPE> &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                return true;
            }

            bool operator <= (const BaseVector4<TYPE> &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                return true;
            }

            bool operator >= (const BaseVector4<TYPE> &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                return true;
            }

            bool operator == (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                return true;
            }

            bool operator != (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                return false;
            }

            BaseVector3 operator = (float scalar)
            {
                x = y = z = scalar;
                return (*this);
            }

            BaseVector3 operator = (const BaseVector2<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = 0.0f;
                return (*this);
            }

            BaseVector3 operator = (const BaseVector3<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            BaseVector3 operator = (const BaseVector4<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            void operator -= (const BaseVector4<TYPE> &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            void operator += (const BaseVector4<TYPE> &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            void operator /= (const BaseVector4<TYPE> &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            void operator *= (const BaseVector4<TYPE> &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            BaseVector3 operator - (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            BaseVector3 operator + (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            BaseVector3 operator / (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            BaseVector3 operator * (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            BaseVector3 operator - (TYPE scalar) const
            {
                return BaseVector3((x - scalar), (y - scalar), (z - scalar));
            }

            BaseVector3 operator + (TYPE scalar) const
            {
                return BaseVector3((x + scalar), (y + scalar), (z + scalar));
            }

            BaseVector3 operator / (TYPE scalar) const
            {
                return BaseVector3((x / scalar), (y / scalar), (z / scalar));
            }

            BaseVector3 operator * (TYPE scalar) const
            {
                return BaseVector3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        template <typename TYPE>
        BaseVector3<TYPE> operator - (const BaseVector4<TYPE> &vector)
        {
            return BaseVector3(-vector.x, -vector.y, -vector.z);
        }

        template <typename TYPE>
        BaseVector3<TYPE> operator - (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector3((scalar - vector.x), (scalar - vector.y), (scalar - vector.z));
        }

        template <typename TYPE>
        BaseVector3<TYPE> operator + (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector3((scalar + vector.x), (scalar + vector.y), (scalar + vector.z));
        }

        template <typename TYPE>
        BaseVector3<TYPE> operator / (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector3((scalar / vector.x), (scalar / vector.y), (scalar / vector.z));
        }

        template <typename TYPE>
        BaseVector3<TYPE> operator * (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector3((scalar * vector.x), (scalar * vector.y), (scalar * vector.z));
        }

        typedef BaseVector3<float> Float3;
    }; // namespace Math
}; // namespace Gek

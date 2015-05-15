#pragma once

#include "GEK\Math\Vector3.h"
namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector2;
        template <typename TYPE> struct BaseVector3;
        template <typename TYPE> struct BaseQuaternion;

        template <typename TYPE>
        struct BaseVector4
        {
        public:
            union
            {
                struct { TYPE x, y, z, w; };
                struct { TYPE r, g, b, a; };
                struct { TYPE xyzw[4]; };
                struct { TYPE rgba[4]; };
                struct { BaseVector3<TYPE> xyz; TYPE w; };
                struct { BaseVector3<TYPE> normal; TYPE distance; };
            };

        public:
            BaseVector4(void)
                : x(0)
                , y(0)
                , z(0)
                , w(1)
            {
            }

            BaseVector4(TYPE scalar)
                : x(scalar)
                , y(scalar)
                , z(scalar)
                , w(scalar)
            {
            }

            BaseVector4(const TYPE *values)
                : x(values[0])
                , y(values[1])
                , z(values[2])
                , w(values[3])
            {
            }

            BaseVector4(const BaseVector2<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(0)
                , w(1)
            {
            }

            BaseVector4(const BaseVector3<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(1)
            {
            }

            BaseVector4(const BaseVector3<TYPE> &vector, TYPE w)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(w)
            {
            }

            BaseVector4(const BaseVector4<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(vector.w)
            {
            }

            BaseVector4(TYPE x, TYPE y, TYPE z, TYPE w)
                : x(x)
                , y(y)
                , z(z)
                , w(w)
            {
            }

            void set(TYPE x, TYPE y, TYPE z, TYPE w)
            {
                this->x = x;
                this->y = y;
                this->z = z;
                this->w = w;
            }

            void setLength(TYPE length)
            {
                (*this) *= (length / getLength());
            }

            TYPE getLengthSquared(void) const
            {
                return ((x * x) + (y * y) + (z * z) + (w * w));
            }

            TYPE getLength(void) const
            {
                return sqrt(getLengthSquared());
            }

            TYPE getMax(void) const
            {
                return max(max(max(x, y), z), w);
            }

            TYPE getDistance(const BaseVector4<TYPE> &vector) const
            {
                return (vector - (*this)).getLength();
            }

            BaseVector4 getNormal(void) const
            {
                TYPE length(getLength());
                if (length != TYPE(0))
                {
                    return ((*this) * (TYPE(1) / length));
                }

                return (*this);
            }

            TYPE dot(const BaseVector4<TYPE> &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
            }

            BaseVector4 lerp(const BaseVector4<TYPE> &vector, TYPE factor) const
            {
                return lerp((*this), vector, factor);
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            TYPE operator [] (int axis) const
            {
                return xyzw[axis];
            }

            TYPE &operator [] (int axis)
            {
                return xyzw[axis];
            }

            bool operator < (const BaseVector4<TYPE> &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                if (w >= vector.w) return false;
                return true;
            }

            bool operator > (const BaseVector4<TYPE> &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                if (w <= vector.w) return false;
                return true;
            }

            bool operator <= (const BaseVector4<TYPE> &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                if (w > vector.w) return false;
                return true;
            }

            bool operator >= (const BaseVector4<TYPE> &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                if (w < vector.w) return false;
                return true;
            }

            bool operator == (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                if (w != vector.w) return false;
                return true;
            }

            bool operator != (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                if (w != vector.w) return true;
                return false;
            }

            BaseVector4 operator = (float scalar)
            {
                x = y = z = w = scalar;
                return (*this);
            }

            BaseVector4 operator = (const BaseVector2<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = TYPE(0);
                w = TYPE(1);
                return (*this);
            }

            BaseVector4 operator = (const BaseVector3<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = TYPE(1);
                return (*this);
            }

            BaseVector4 operator = (const BaseVector4<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = vector.w;
                return (*this);
            }

            void operator -= (const BaseVector4<TYPE> &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
                w -= vector.w;
            }

            void operator += (const BaseVector4<TYPE> &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
                w += vector.w;
            }

            void operator /= (const BaseVector4<TYPE> &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
                w /= vector.w;
            }

            void operator *= (const BaseVector4<TYPE> &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
                w *= vector.w;
            }

            void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
                w -= scalar;
            }

            void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
                w += scalar;
            }

            void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
                w /= scalar;
            }

            void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                w *= scalar;
            }

            BaseVector4 operator - (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector4((x - vector.x), (y - vector.y), (z - vector.z), (w - vector.w));
            }

            BaseVector4 operator + (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector4((x + vector.x), (y + vector.y), (z + vector.z), (w + vector.w));
            }

            BaseVector4 operator / (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector4((x / vector.x), (y / vector.y), (z / vector.z), (w / vector.w));
            }

            BaseVector4 operator * (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector4((x * vector.x), (y * vector.y), (z * vector.z), (w * vector.w));
            }

            BaseVector4 operator - (TYPE scalar) const
            {
                return BaseVector4((x - scalar), (y - scalar), (z - scalar), (w - scalar));
            }

            BaseVector4 operator + (TYPE scalar) const
            {
                return BaseVector4((x + scalar), (y + scalar), (z + scalar), (w + scalar));
            }

            BaseVector4 operator / (TYPE scalar) const
            {
                return BaseVector4((x / scalar), (y / scalar), (z / scalar), (w / scalar));
            }

            BaseVector4 operator * (TYPE scalar) const
            {
                return BaseVector4((x * scalar), (y * scalar), (z * scalar), (w * scalar));
            }
        };

        template <typename TYPE>
        BaseVector4<TYPE> operator - (const BaseVector4<TYPE> &vector)
        {
            return BaseVector4(-vector.x, -vector.y, -vector.z, -vector.w);
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator - (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector4((scalar - vector.x), (scalar - vector.y), (scalar - vector.z), (scalar - vector.w));
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator + (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector4((scalar + vector.x), (scalar + vector.y), (scalar + vector.z), (scalar + vector.w));
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator / (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector4((scalar / vector.x), (scalar / vector.y), (scalar / vector.z), (scalar / vector.w));
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator * (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
            return BaseVector4((scalar * vector.x), (scalar * vector.y), (scalar * vector.z), (scalar * vector.w));
        }

        typedef BaseVector4<float> Float4;
    }; // namespace Math
}; // namespace Gek

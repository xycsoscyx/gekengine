#pragma once

#include <cmath>
#include <algorithm>
#include <initializer_list>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector3;
        template <typename TYPE> struct BaseVector4;

        template <typename TYPE>
        struct BaseVector2
        {
        public:
            union
            {
                struct { TYPE x, y; };
                struct { TYPE u, v; };
                struct { TYPE xy[2]; };
                struct { TYPE uv[2]; };
                struct { TYPE data[2]; };
            };

        public:
            BaseVector2(void)
                : x(0)
                , y(0)
            {
            }

            BaseVector2(const std::initializer_list<float> &list)
            {
                std::copy(list.begin(), list.end(), data);
            }

            BaseVector2(const TYPE *vector)
            {
                std::copy_n(vector, 2, data);
            }

            BaseVector2(TYPE scalar)
                : x(value)
                , y(value)
            {
                x = y = scalar;
            }

            BaseVector2(const BaseVector2<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
            {
            }

            BaseVector2(const BaseVector3<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
            {
            }

            BaseVector2(const BaseVector4<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
            {
            }

            BaseVector2(TYPE x, TYPE y)
                : x(x)
                , y(y)
            {
            }

            void set(TYPE x, TYPE y)
            {
                this->x = x;
                this->y = y;
            }

            void setLength(TYPE length)
            {
                (*this) *= (length / getLength());
            }

            TYPE getLengthSquared(void) const
            {
                return ((x * x) + (y * y));
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            TYPE getMax(void) const
            {
                return std::max(x, y);
            }

            TYPE getDistance(const BaseVector2<TYPE> &vector) const
            {
                return (vector - (*this)).getLength();
            }

            BaseVector2 getNormal(void) const
            {
                TYPE length = getLength();
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

            TYPE dot(const BaseVector2<TYPE> &vector) const
            {
                return ((x * vector.x) + (y * vector.y));
            }

            BaseVector2 lerp(const BaseVector2<TYPE> &vector, TYPE factor) const
            {
                return lerp((*this), vector, factor);
            }

            TYPE operator [] (int axis) const
            {
                return xy[axis];
            }

            TYPE &operator [] (int axis)
            {
                return xy[axis];
            }

            bool operator < (const BaseVector2<TYPE> &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                return true;
            }

            bool operator > (const BaseVector2<TYPE> &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                return true;
            }

            bool operator <= (const BaseVector2<TYPE> &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                return true;
            }

            bool operator >= (const BaseVector2<TYPE> &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                return true;
            }

            bool operator == (const BaseVector2<TYPE> &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                return true;
            }

            bool operator != (const BaseVector2<TYPE> &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                return false;
            }

            BaseVector2 operator = (float scalar)
            {
                x = y = scalar;
                return (*this);
            }

            BaseVector2 operator = (const BaseVector2<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            BaseVector2 operator = (const BaseVector3<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            BaseVector2 operator = (const BaseVector4<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            void operator -= (const BaseVector2<TYPE> &vector)
            {
                x -= vector.x;
                y -= vector.y;
            }

            void operator += (const BaseVector2<TYPE> &vector)
            {
                x += vector.x;
                y += vector.y;
            }

            void operator /= (const BaseVector2<TYPE> &vector)
            {
                x /= vector.x;
                y /= vector.y;
            }

            void operator *= (const BaseVector2<TYPE> &vector)
            {
                x *= vector.x;
                y *= vector.y;
            }

            void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
            }

            void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
            }

            void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
            }

            void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
            }

            BaseVector2 operator - (const BaseVector2<TYPE> &vector) const
            {
                return BaseVector2((x - vector.x), (y - vector.y));
            }

            BaseVector2 operator + (const BaseVector2<TYPE> &vector) const
            {
                return BaseVector2((x + vector.x), (y + vector.y));
            }

            BaseVector2 operator / (const BaseVector2<TYPE> &vector) const
            {
                return BaseVector2((x / vector.x), (y / vector.y));
            }

            BaseVector2 operator * (const BaseVector2<TYPE> &vector) const
            {
                return BaseVector2((x * vector.x), (y * vector.y));
            }

            BaseVector2 operator - (TYPE scalar) const
            {
                return BaseVector2((x - scalar), (y - scalar));
            }

            BaseVector2 operator + (TYPE scalar) const
            {
                return BaseVector2((x + scalar), (y + scalar));
            }

            BaseVector2 operator / (TYPE scalar) const
            {
                return BaseVector2((x / scalar), (y / scalar));
            }

            BaseVector2 operator * (TYPE scalar) const
            {
                return BaseVector2((x * scalar), (y * scalar));
            }
        };

        template <typename TYPE>
        BaseVector2<TYPE> operator - (const BaseVector2<TYPE> &vector)
        {
            return BaseVector2(-vector.x, -vector.y);
        }

        template <typename TYPE>
        BaseVector2<TYPE> operator - (TYPE scalar, const BaseVector2<TYPE> &vector)
        {
            return BaseVector2((scalar - vector.x), (scalar - vector.y));
        }

        template <typename TYPE>
        BaseVector2<TYPE> operator + (TYPE scalar, const BaseVector2<TYPE> &vector)
        {
            return BaseVector2((scalar + vector.x), (scalar + vector.y));
        }

        template <typename TYPE>
        BaseVector2<TYPE> operator / (TYPE scalar, const BaseVector2<TYPE> &vector)
        {
            return BaseVector2((scalar / vector.x), (scalar / vector.y));
        }

        template <typename TYPE>
        BaseVector2<TYPE> operator * (TYPE scalar, const BaseVector2<TYPE> &vector)
        {
            return BaseVector2((scalar * vector.x), (scalar * vector.y));
        }

        typedef BaseVector2<float> Float2;
    }; // namespace Math
}; // namespace Gek

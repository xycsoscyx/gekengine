/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

namespace Gek
{
    namespace Math
    {
        template <typename TYPE>
        struct Vector2
        {
        public:
            static const Vector2 Zero;
            static const Vector2 One;

        public:
            union
            {
                struct { TYPE x, y; };
                struct { TYPE u, v; };
                struct { TYPE data[2]; };
            };

        public:
            inline Vector2(void)
            {
            }

            inline Vector2(const TYPE(&data)[2])
                : data{ data[0], data[1] }
            {
            }

            inline Vector2(const TYPE *data)
                : data{ data[0], data[1] }
            {
            }

            inline Vector2(TYPE scalar)
                : data{ scalar, scalar }
            {
            }

            inline Vector2(const Vector2 &vector)
                : data{ vector.data[0], vector.data[1] }
            {
            }

            inline Vector2(TYPE x, TYPE y)
                : data{ x, y }
            {
            }

            inline void set(TYPE x, TYPE y)
            {
                this->x = x;
                this->y = y;
            }

            inline void set(TYPE value)
            {
                this->x = value;
                this->y = value;
            }

            TYPE getLengthSquared(void) const
            {
                return ((x * x) + (y * y));
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            TYPE getDistance(const Vector2 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Vector2 getNormal(void) const
            {
                return ((*this) / getLength());
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            TYPE dot(const Vector2 &vector) const
            {
                return ((x * vector.x) + (y * vector.y));
            }

            Vector2 lerp(const Vector2 &vector, TYPE factor) const
            {
                return Math::lerp((*this), vector, factor);
            }

            bool operator < (const Vector2 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                return true;
            }

            bool operator > (const Vector2 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                return true;
            }

            bool operator <= (const Vector2 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                return true;
            }

            bool operator >= (const Vector2 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                return true;
            }

            bool operator == (const Vector2 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                return true;
            }

            bool operator != (const Vector2 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                return false;
            }

            inline TYPE operator [] (int axis) const
            {
                return data[axis];
            }

            inline TYPE &operator [] (int axis)
            {
                return data[axis];
            }

            inline operator const TYPE *() const
            {
                return data;
            }

            inline operator TYPE *()
            {
                return data;
            }

            // vector operations
            inline Vector2 &operator = (const Vector2 &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            inline void operator -= (const Vector2 &vector)
            {
                x -= vector.x;
                y -= vector.y;
            }

            inline void operator += (const Vector2 &vector)
            {
                x += vector.x;
                y += vector.y;
            }

            inline void operator /= (const Vector2 &vector)
            {
                x /= vector.x;
                y /= vector.y;
            }

            inline void operator *= (const Vector2 &vector)
            {
                x *= vector.x;
                y *= vector.y;
            }

            inline Vector2 operator - (const Vector2 &vector) const
            {
                return Vector2((x - vector.x), (y - vector.y));
            }

            inline Vector2 operator + (const Vector2 &vector) const
            {
                return Vector2((x + vector.x), (y + vector.y));
            }

            inline Vector2 operator / (const Vector2 &vector) const
            {
                return Vector2((x / vector.x), (y / vector.y));
            }

            inline Vector2 operator * (const Vector2 &vector) const
            {
                return Vector2((x * vector.x), (y * vector.y));
            }

            // scalar operations
            inline Vector2 &operator = (TYPE scalar)
            {
                x = scalar;
                y = scalar;
                return (*this);
            }

            inline void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
            }

            inline void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
            }

            inline void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
            }

            inline void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
            }

            inline Vector2 operator - (TYPE scalar) const
            {
                return Vector2((x - scalar), (y - scalar));
            }

            inline Vector2 operator + (TYPE scalar) const
            {
                return Vector2((x + scalar), (y + scalar));
            }

            inline Vector2 operator / (TYPE scalar) const
            {
                return Vector2((x / scalar), (y / scalar));
            }

            inline Vector2 operator * (TYPE scalar) const
            {
                return Vector2((x * scalar), (y * scalar));
            }
        };

        template <typename TYPE>
        inline Vector2<TYPE> operator - (const Vector2<TYPE> &vector)
        {
            return Vector2(-vector.x, -vector.y);
        }

        template <typename TYPE>
        inline Vector2<TYPE> operator + (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar + vector.x, scalar + vector.y);
        }

        template <typename TYPE>
        inline Vector2<TYPE> operator - (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar - vector.x, scalar - vector.y);
        }

        template <typename TYPE>
        inline Vector2<TYPE> operator * (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar * vector.x, scalar * vector.y);
        }

        template <typename TYPE>
        inline Vector2<TYPE> operator / (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar / vector.x, scalar / vector.y);
        }

        using Float2 = Vector2<float>;
        using Int2 = Vector2<int>;
        using UInt2 = Vector2<unsigned int>;
    }; // namespace Math
}; // namespace Gek

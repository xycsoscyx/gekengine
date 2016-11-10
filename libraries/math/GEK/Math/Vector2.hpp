/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include <type_traits>
#include <cstdint>
#include <cmath>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
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
                struct { TYPE minimum, maximum; };
                struct { TYPE data[2]; };
            };

        public:
            Vector2(void)
            {
            }

            Vector2(TYPE scalar)
                : data{ TYPE(scalar), TYPE(scalar) }
            {
            }

            Vector2(TYPE x, TYPE y)
                : data{ TYPE(x), TYPE(y) }
            {
            }

            Vector2(const TYPE *data)
                : data{ TYPE(data[0]), TYPE(data[1]) }
            {
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector2(const Vector2<OTHER> &vector)
                : data{ TYPE(vector.data[0]), TYPE(vector.data[1]) }
            {
            }

            void set(TYPE value)
            {
                this->x = this->y = TYPE(value);
            }

            void set(TYPE x, TYPE y)
            {
                this->x = TYPE(x);
                this->y = TYPE(y);
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

			Vector2 getMinimum(const Vector2 &vector) const
			{
				return Vector2(
					std::min(x, vector.x),
					std::min(y, vector.y)
				);
			}

			Vector2 getMaximum(const Vector2 &vector) const
			{
				return Vector2(
					std::max(x, vector.x),
					std::max(y, vector.y)
				);
			}

			Vector2 getClamped(const Vector2 &min, const Vector2 &max) const
			{
				return Vector2(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y)
				);
			}

			Vector2 getSaturated(void) const
			{
				return getClamped(Zero, One);
			}

            TYPE dot(const Vector2 &vector) const
            {
                return ((x * vector.x) + (y * vector.y));
            }

            Vector2 lerp(const Vector2 &vector, TYPE factor) const
            {
                return Math::Utility::lerp((*this), vector, factor);
            }

			void normalize(void)
			{
				(*this) = getNormal();
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

            TYPE operator [] (int axis) const
            {
                return data[axis];
            }

            TYPE &operator [] (int axis)
            {
                return data[axis];
            }

            operator const TYPE *() const
            {
                return data;
            }

            operator TYPE *()
            {
                return data;
            }

            // vector operations
            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector2 &operator = (const Vector2<OTHER> &vector)
            {
                x = TYPE(vector.x);
                y = TYPE(vector.y);
                return (*this);
            }

            void operator -= (const Vector2 &vector)
            {
                x -= vector.x;
                y -= vector.y;
            }

            void operator += (const Vector2 &vector)
            {
                x += vector.x;
                y += vector.y;
            }

            void operator /= (const Vector2 &vector)
            {
                x /= vector.x;
                y /= vector.y;
            }

            void operator *= (const Vector2 &vector)
            {
                x *= vector.x;
                y *= vector.y;
            }

            Vector2 operator - (const Vector2 &vector) const
            {
                return Vector2((x - vector.x), (y - vector.y));
            }

            Vector2 operator + (const Vector2 &vector) const
            {
                return Vector2((x + vector.x), (y + vector.y));
            }

            Vector2 operator / (const Vector2 &vector) const
            {
                return Vector2((x / vector.x), (y / vector.y));
            }

            Vector2 operator * (const Vector2 &vector) const
            {
                return Vector2((x * vector.x), (y * vector.y));
            }

            // scalar operations
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

            Vector2 operator - (TYPE scalar) const
            {
                return Vector2((x - scalar), (y - scalar));
            }

            Vector2 operator + (TYPE scalar) const
            {
                return Vector2((x + scalar), (y + scalar));
            }

            Vector2 operator / (TYPE scalar) const
            {
                return Vector2((x / scalar), (y / scalar));
            }

            Vector2 operator * (TYPE scalar) const
            {
                return Vector2((x * scalar), (y * scalar));
            }
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator - (const Vector2<TYPE> &vector)
        {
            return Vector2(-vector.x, -vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator + (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar + vector.x, scalar + vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator - (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar - vector.x, scalar - vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator * (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar * vector.x, scalar * vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator / (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2(scalar / vector.x, scalar / vector.y);
        }

        using Float2 = Vector2<float>;
        using Int2 = Vector2<int32_t>;
        using UInt2 = Vector2<uint32_t>;
    }; // namespace Math
}; // namespace Gek

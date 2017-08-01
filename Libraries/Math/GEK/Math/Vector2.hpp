/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Common.hpp"
#include <tuple>

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
                struct { TYPE width, height; };
                struct { TYPE minimum, maximum; };
                struct { TYPE data[2]; };
            };

        public:
            Vector2(void)
            {
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector2(const Vector2<OTHER> &vector)
                : x(TYPE(vector.x))
                , y(TYPE(vector.y))
            {
            }

            explicit Vector2(TYPE scalar)
                : data{ TYPE(scalar), TYPE(scalar) }
            {
            }

            explicit Vector2(TYPE x, TYPE y)
                : data{ TYPE(x), TYPE(y) }
            {
            }

            explicit Vector2(const TYPE *data)
                : data{ TYPE(data[0]), TYPE(data[1]) }
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

            void set(const TYPE *data)
            {
                this->x = TYPE(data[0]);
                this->y = TYPE(data[1]);
            }

            TYPE getMagnitude(void) const
            {
                return dot(*this);
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getMagnitude());
            }

            TYPE getDistance(const Vector2 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Vector2<TYPE> getNormal(void) const
            {
                float inverseLength = (1.0f / getLength());
                return ((*this) * inverseLength);
            }

            Vector2<TYPE> getAbsolute(void) const
            {
                return Vector2(
                    std::abs(x),
                    std::abs(y));
            }

            Vector2<TYPE> getMinimum(const Vector2 &vector) const
			{
				return Vector2<TYPE>(
					std::min(x, vector.x),
					std::min(y, vector.y)
				);
			}

			Vector2 getMaximum(const Vector2<TYPE> &vector) const
			{
				return Vector2(
					std::max(x, vector.x),
					std::max(y, vector.y)
				);
			}

			Vector2<TYPE> getClamped(const Vector2 &min, const Vector2 &max) const
			{
				return Vector2(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y)
				);
			}

			Vector2<TYPE> getSaturated(void) const
			{
				return getClamped(Zero, One);
			}

            TYPE dot(const Vector2 &vector) const
            {
                return ((x * vector.x) + (y * vector.y));
            }

			void normalize(void)
			{
                float inverseLength = (1.0f / getLength());
                (*this) *= inverseLength;
            }

            std::tuple<TYPE, TYPE> getTuple(void) const
            {
                return std::make_tuple(x, y);
            }

			bool operator < (const Vector2 &vector) const
            {
                return (getTuple() < vector.getTuple());
            }

            bool operator > (const Vector2 &vector) const
            {
                return (getTuple() > vector.getTuple());
            }

            bool operator <= (const Vector2 &vector) const
            {
                return (getTuple() <= vector.getTuple());
            }

            bool operator >= (const Vector2 &vector) const
            {
                return (getTuple() >= vector.getTuple());
            }

            bool operator == (const Vector2 &vector) const
            {
                return (getTuple() == vector.getTuple());
            }

            bool operator != (const Vector2 &vector) const
            {
                return (getTuple() != vector.getTuple());
            }

            // vector operations
            float &operator [] (size_t index)
            {
                return data[index];
            }

            float const &operator [] (size_t index) const
            {
                return data[index];
            }

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
            return Vector2<TYPE>(-vector.x, -vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator + (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2<TYPE>(scalar + vector.x, scalar + vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator - (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2<TYPE>(scalar - vector.x, scalar - vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator * (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2<TYPE>(scalar * vector.x, scalar * vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator / (TYPE scalar, const Vector2<TYPE> &vector)
        {
            return Vector2<TYPE>(scalar / vector.x, scalar / vector.y);
        }

        using Float2 = Vector2<float>;
        using Int2 = Vector2<int32_t>;
        using UInt2 = Vector2<uint32_t>;
    }; // namespace Math
}; // namespace Gek

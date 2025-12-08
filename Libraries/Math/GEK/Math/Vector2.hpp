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
            using TYPE2 = Vector2<TYPE>;

            static const TYPE2 Zero;
            static const TYPE2 One;

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
            Vector2(void) noexcept
            {
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector2(Vector2<OTHER> const &vector) noexcept
                : x(TYPE(vector.x))
                , y(TYPE(vector.y))
            {
            }

            explicit Vector2(TYPE scalar) noexcept
                : data{ scalar, scalar }
            {
            }

            explicit Vector2(TYPE x, TYPE y) noexcept
                : data{ x, y }
            {
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            explicit Vector2(OTHER x, OTHER y) noexcept
                : data{ TYPE(x), TYPE(y) }
            {
            }

            explicit Vector2(TYPE const * const data) noexcept
                : data{ TYPE(data[0]), TYPE(data[1]) }
            {
            }

            void set(TYPE value) noexcept
            {
                this->x = this->y = TYPE(value);
            }

            void set(TYPE x, TYPE y) noexcept
            {
                this->x = TYPE(x);
                this->y = TYPE(y);
            }

            void set(TYPE const * const data) noexcept
            {
                this->x = TYPE(data[0]);
                this->y = TYPE(data[1]);
            }

            TYPE getMagnitude(void) const noexcept
            {
                return dot(*this);
            }

            TYPE getLength(void) const noexcept
            {
                return std::sqrt(getMagnitude());
            }

            TYPE getDistance(TYPE2 const &vector) const noexcept
            {
                return (vector - (*this)).getLength();
            }

            TYPE2 getNormal(void) const noexcept
            {
                float inverseLength = (1.0f / getLength());
                return ((*this) * inverseLength);
            }

            TYPE2 getAbsolute(void) const noexcept
            {
                return TYPE2(
                    std::abs(x),
                    std::abs(y));
            }

            TYPE2 getMinimum(TYPE2 const &vector) const noexcept
			{
				return TYPE2(
					std::min(x, vector.x),
					std::min(y, vector.y)
				);
			}

			TYPE2 getMaximum(TYPE2 const &vector) const noexcept
			{
				return TYPE2(
					std::max(x, vector.x),
					std::max(y, vector.y)
				);
			}

			TYPE2 getClamped(TYPE2 const &min, TYPE2 const &max) const noexcept
			{
				return TYPE2(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y)
				);
			}

			TYPE2 getSaturated(void) const noexcept
			{
				return getClamped(Zero, One);
			}

            TYPE dot(TYPE2 const &vector) const noexcept
            {
                return ((x * vector.x) + (y * vector.y));
            }

			void normalize(void) noexcept
			{
                float inverseLength = (1.0f / getLength());
                (*this) *= inverseLength;
            }

            std::tuple<TYPE, TYPE> getTuple(void) const noexcept
            {
                return std::make_tuple(x, y);
            }

			bool operator < (TYPE2 const &vector) const noexcept
            {
                return (getTuple() < vector.getTuple());
            }

            bool operator > (TYPE2 const &vector) const noexcept
            {
                return (getTuple() > vector.getTuple());
            }

            bool operator <= (TYPE2 const &vector) const noexcept
            {
                return (getTuple() <= vector.getTuple());
            }

            bool operator >= (TYPE2 const &vector) const noexcept
            {
                return (getTuple() >= vector.getTuple());
            }

            bool operator == (TYPE2 const &vector) const noexcept
            {
                return (getTuple() == vector.getTuple());
            }

            bool operator != (TYPE2 const &vector) const noexcept
            {
                return (getTuple() != vector.getTuple());
            }

            // vector operations
            float &operator [] (size_t index) noexcept
            {
                return data[index];
            }

            float const &operator [] (size_t index) const noexcept
            {
                return data[index];
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            TYPE2 &operator = (Vector2<OTHER> const &vector) noexcept
            {
                x = TYPE(vector.x);
                y = TYPE(vector.y);
                return (*this);
            }

            void operator -= (TYPE2 const &vector) noexcept
            {
                x -= vector.x;
                y -= vector.y;
            }

            void operator += (TYPE2 const &vector) noexcept
            {
                x += vector.x;
                y += vector.y;
            }

            void operator /= (TYPE2 const &vector) noexcept
            {
                x /= vector.x;
                y /= vector.y;
            }

            void operator *= (TYPE2 const &vector) noexcept
            {
                x *= vector.x;
                y *= vector.y;
            }

            TYPE2 operator - (TYPE2 const &vector) const noexcept
            {
                return TYPE2((x - vector.x), (y - vector.y));
            }

            TYPE2 operator + (TYPE2 const &vector) const noexcept
            {
                return TYPE2((x + vector.x), (y + vector.y));
            }

            TYPE2 operator / (TYPE2 const &vector) const noexcept
            {
                return TYPE2((x / vector.x), (y / vector.y));
            }

            TYPE2 operator * (TYPE2 const &vector) const noexcept
            {
                return TYPE2((x * vector.x), (y * vector.y));
            }

            // scalar operations
            void operator -= (TYPE scalar) noexcept
            {
                x -= scalar;
                y -= scalar;
            }

            void operator += (TYPE scalar) noexcept
            {
                x += scalar;
                y += scalar;
            }

            void operator /= (TYPE scalar) noexcept
            {
                x /= scalar;
                y /= scalar;
            }

            void operator *= (TYPE scalar) noexcept
            {
                x *= scalar;
                y *= scalar;
            }

            TYPE2 operator - (TYPE scalar) const noexcept
            {
                return TYPE2((x - scalar), (y - scalar));
            }

            TYPE2 operator + (TYPE scalar) const noexcept
            {
                return TYPE2((x + scalar), (y + scalar));
            }

            TYPE2 operator / (TYPE scalar) const noexcept
            {
                return TYPE2((x / scalar), (y / scalar));
            }

            TYPE2 operator * (TYPE scalar) const noexcept
            {
                return TYPE2((x * scalar), (y * scalar));
            }
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator - (Vector2<TYPE> const &vector) noexcept
        {
            return Vector2<TYPE>(-vector.x, -vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator + (TYPE scalar, Vector2<TYPE> const &vector) noexcept
        {
            return Vector2<TYPE>(scalar + vector.x, scalar + vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator - (TYPE scalar, Vector2<TYPE> const &vector) noexcept
        {
            return Vector2<TYPE>(scalar - vector.x, scalar - vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator * (TYPE scalar, Vector2<TYPE> const &vector) noexcept
        {
            return Vector2<TYPE>(scalar * vector.x, scalar * vector.y);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector2<TYPE> operator / (TYPE scalar, Vector2<TYPE> const &vector) noexcept
        {
            return Vector2<TYPE>(scalar / vector.x, scalar / vector.y);
        }

        using Float2 = Vector2<float>;
        using Int2 = Vector2<int32_t>;
        using UInt2 = Vector2<uint32_t>;
    }; // namespace Math
}; // namespace Gek

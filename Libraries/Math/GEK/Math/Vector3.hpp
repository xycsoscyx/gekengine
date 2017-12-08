/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector2.hpp"

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        struct Vector3
        {
        public:
            using TYPE2 = Vector2<TYPE>;
            using TYPE3 = Vector3<TYPE>;

            static const TYPE3 Zero;
            static const TYPE3 One;

        public:
            union
            {
				struct { TYPE data[3]; };

                struct
                {
                    union
                    {
                        struct { TYPE x, y; };
                        TYPE2 xy;
                    };
                    
                    TYPE z;
                };
            };

        public:
            Vector3(void) noexcept
            {
            }

            Vector3(TYPE3 const &vector) noexcept
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
            {
            }

            explicit Vector3(TYPE scalar) noexcept
                : data{ scalar, scalar, scalar }
            {
            }

            explicit Vector3(TYPE x, TYPE y, TYPE z) noexcept
                : data{ x, y, z }
            {
            }

            explicit Vector3(const TYPE *data) noexcept
                : data{ data[0], data[1], data[2] }
            {
            }

            Vector3(TYPE2 const &xy, TYPE z) noexcept
                : x(vector.x)
                , y(vector.y)
                , z(z)
            {
            }

            void set(TYPE value) noexcept
            {
                this->x = this->y = this->z = value;
            }

            void set(TYPE x, TYPE y, TYPE z) noexcept
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            void set(const TYPE *data) noexcept
            {
                this->x = data[0];
                this->y = data[1];
                this->z = data[2];
            }

            TYPE getMagnitude(void) const noexcept
            {
                return dot(*this);
            }

            TYPE getLength(void) const noexcept
            {
                return std::sqrt(getMagnitude());
            }

            TYPE getDistance(TYPE3 const &vector) const noexcept
            {
                return (vector - (*this)).getLength();
            }

            TYPE3 getNormal(void) const noexcept
            {
                float inverseLength = (1.0f / getLength());
                return ((*this) * inverseLength);
            }

            TYPE3 getAbsolute(void) const noexcept
            {
                return TYPE3(
                    std::abs(x),
                    std::abs(y),
                    std::abs(z));
            }

            TYPE3 getMinimum(TYPE3 const &vector) const noexcept
			{
				return TYPE3(
					std::min(x, vector.x),
					std::min(y, vector.y),
					std::min(z, vector.z)
				);
			}

			TYPE3 getMaximum(TYPE3 const &vector) const noexcept
			{
				return TYPE3(
					std::max(x, vector.x),
					std::max(y, vector.y),
					std::max(z, vector.z)
				);
			}

			TYPE3 getClamped(TYPE3 const &min, TYPE3 const &max) const noexcept
			{
				return TYPE3(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y),
					std::min(std::max(z, min.z), max.z)
				);
			}

			TYPE3 getSaturated(void) const noexcept
			{
				return getClamped(Zero, One);
			}

			TYPE dot(TYPE3 const &vector) const noexcept
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z));
            }

            TYPE3 cross(TYPE3 const &vector) const noexcept
            {
                return TYPE3(
                    ((y * vector.z) - (z * vector.y)),
                    ((z * vector.x) - (x * vector.z)),
                    ((x * vector.y) - (y * vector.x)));
            }

            void normalize(void) noexcept
            {
                float inverseLength = (1.0f / getLength());
                (*this) *= inverseLength;
            }

            std::tuple<TYPE, TYPE, TYPE> getTuple(void) const noexcept
            {
                return std::make_tuple(x, y, z);
            }

            bool operator < (TYPE3 const &vector) const noexcept
            {
                return (getTuple() < vector.getTuple());
            }

            bool operator > (TYPE3 const &vector) const noexcept
            {
                return (getTuple() > vector.getTuple());
            }

            bool operator <= (TYPE3 const &vector) const noexcept
            {
                return (getTuple() <= vector.getTuple());
            }

            bool operator >= (TYPE3 const &vector) const noexcept
            {
                return (getTuple() >= vector.getTuple());
            }

            bool operator == (TYPE3 const &vector) const noexcept
            {
                return (getTuple() == vector.getTuple());
            }

            bool operator != (TYPE3 const &vector) const noexcept
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

            TYPE3 &operator = (TYPE3 const &vector) noexcept
            {
                std::tie(x, y, z) = vector.getTuple();
                return (*this);
            }

            void operator -= (TYPE3 const &vector) noexcept
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            void operator += (TYPE3 const &vector) noexcept
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            void operator /= (TYPE3 const &vector) noexcept
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            void operator *= (TYPE3 const &vector) noexcept
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            TYPE3 operator - (TYPE3 const &vector) const noexcept
            {
                return TYPE3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            TYPE3 operator + (TYPE3 const &vector) const noexcept
            {
                return TYPE3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            TYPE3 operator / (TYPE3 const &vector) const noexcept
            {
                return TYPE3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            TYPE3 operator * (TYPE3 const &vector) const noexcept
            {
                return TYPE3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            // scalar operations
            void operator -= (TYPE scalar) noexcept
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            void operator += (TYPE scalar) noexcept
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            void operator /= (TYPE scalar) noexcept
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            void operator *= (TYPE scalar) noexcept
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            TYPE3 operator - (TYPE scalar) const noexcept
            {
                return TYPE3((x - scalar), (y - scalar), (z - scalar));
            }

            TYPE3 operator + (TYPE scalar) const noexcept
            {
                return TYPE3((x + scalar), (y + scalar), (z + scalar));
            }

            TYPE3 operator / (TYPE scalar) const noexcept
            {
                return TYPE3((x / scalar), (y / scalar), (z / scalar));
            }

            TYPE3 operator * (TYPE scalar) const noexcept
            {
                return TYPE3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator - (Vector3<TYPE> const &vector) noexcept
        {
            return Vector3<TYPE>(-vector.x, -vector.y, -vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator + (TYPE scalar, Vector3<TYPE> const &vector) noexcept
        {
            return Vector3<TYPE>(scalar + vector.x, scalar + vector.y, scalar + vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator - (TYPE scalar, Vector3<TYPE> const &vector) noexcept
        {
            return Vector3<TYPE>(scalar - vector.x, scalar - vector.y, scalar - vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator * (TYPE scalar, Vector3<TYPE> const &vector) noexcept
        {
            return Vector3<TYPE>(scalar * vector.x, scalar * vector.y, scalar * vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator / (TYPE scalar, Vector3<TYPE> const &vector) noexcept
        {
            return Vector3<TYPE>(scalar / vector.x, scalar / vector.y, scalar / vector.z);
        }

        using Float3 = Vector3<float>;
        using Int3 = Vector3<int32_t>;
        using UInt3 = Vector3<uint32_t>;
    }; // namespace Math
}; // namespace Gek

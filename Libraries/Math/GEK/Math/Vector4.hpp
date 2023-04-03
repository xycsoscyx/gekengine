/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        struct Vector4
        {
        public:
            using TYPE2 = Vector2<TYPE>;
            using TYPE3 = Vector3<TYPE>;
            using TYPE4 = Vector4<TYPE>;

            static const TYPE4 Zero;
			static const TYPE4 One;
			static const TYPE4 Black;
			static const TYPE4 White;

        public:
            union
            {
				struct { TYPE data[4]; };
                struct
                {
                    union
                    {
                        struct { TYPE x, y, z; };
                        struct { TYPE r, g, b; };
                        TYPE3 xyz;
                        TYPE3 rgb;
                    };
                    
                    union
                    {
                        TYPE w;
                        TYPE a;
                    };
                };

                struct
                {
                    TYPE2 minimum;
                    TYPE2 maximum;
                };

                struct
                {
                    TYPE2 xy;
                    TYPE2 zw;
                };

                struct
                {
                    TYPE2 position;
                    TYPE2 size;
                };
            };

        public:
            Vector4(void) noexcept
            {
            }

            Vector4(TYPE4 const &vector) noexcept
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(vector.w)
            {
            }

            explicit Vector4(TYPE value) noexcept
				: x(value)
				, y(value)
				, z(value)
				, w(value)
            {
            }

            explicit Vector4(TYPE x, TYPE y, TYPE z, TYPE w) noexcept
				: x(x)
				, y(y)
				, z(z)
				, w(w)
			{
            }

            explicit Vector4(TYPE const *data) noexcept
				: x(data[0])
				, y(data[1])
				, z(data[2])
				, w(data[3])
			{
            }

            explicit Vector4(TYPE3 const &xyz, TYPE w) noexcept
				: x(xyz.x)
				, y(xyz.y)
				, z(xyz.z)
				, w(w)
			{
			}

            explicit Vector4(TYPE2 const &xy, TYPE2 const &zw) noexcept
				: x(xy.x)
				, y(xy.y)
				, z(zw.x)
				, w(zw.y)
			{
			}

            void set(TYPE value) noexcept
            {
                x = y = z = w = value;
            }

            void set(TYPE x, TYPE y, TYPE z, TYPE w) noexcept
            {
				this->x = x;
				this->y = y;
				this->z = z;
				this->w = w;
            }

            void set(TYPE const *data) noexcept
            {
                this->x = data[0];
                this->y = data[1];
                this->z = data[2];
                this->w = data[3];
            }

            void set(TYPE3 const &xyz, float w) noexcept
            {
                this->xyz = xyz;
                this->w = w;
            }

            TYPE getMagnitude(void) const noexcept
            {
                return dot(*this);
            }

            TYPE getLength(void) const noexcept
            {
                return std::sqrt(getMagnitude());
            }

            TYPE getDistance(TYPE4 const &vector) const noexcept
            {
                return (vector - (*this)).getLength();
            }

            TYPE4 getNormal(void) const noexcept
            {
                float inverseLength = (1.0f / getLength());
                return ((*this) * inverseLength);
            }

            TYPE4 getAbsolute(void) const noexcept
            {
                return TYPE4(
                    std::abs(x),
                    std::abs(y),
                    std::abs(z),
                    std::abs(w));
            }

            TYPE4 getMinimum(TYPE4 const &vector) const noexcept
			{
				return TYPE4(
					std::min(x, vector.x),
					std::min(y, vector.y),
					std::min(z, vector.z),
					std::min(w, vector.w)
				);
			}

			TYPE4 getMaximum(TYPE4 const &vector) const noexcept
			{
				return TYPE4(
					std::max(x, vector.x),
					std::max(y, vector.y),
					std::max(z, vector.z),
					std::max(w, vector.w)
				);
			}

			TYPE4 getClamped(TYPE4 const &min, TYPE4 const &max) const noexcept
			{
				return TYPE4(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y),
					std::min(std::max(z, min.z), max.z),
					std::min(std::max(w, min.w), max.w)
				);
			}

			TYPE4 getSaturated(void) const noexcept
			{
				return getClamped(Zero, One);
			}

			TYPE dot(TYPE4 const &vector) const noexcept
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
            }

			void normalize(void) noexcept
			{
                float inverseLength = (1.0f / getLength());
                (*this) *= inverseLength;
			}

            std::tuple<TYPE, TYPE, TYPE, TYPE> getTuple(void) const noexcept
            {
                return std::make_tuple(x, y, z, w);
            }

            bool operator < (TYPE4 const &vector) const noexcept
            {
                return (getTuple() < vector.getTuple());
            }

            bool operator > (TYPE4 const &vector) const noexcept
            {
                return (getTuple() > vector.getTuple());
            }

            bool operator <= (TYPE4 const &vector) const noexcept
            {
                return (getTuple() <= vector.getTuple());
            }

            bool operator >= (TYPE4 const &vector) const noexcept
            {
                return (getTuple() >= vector.getTuple());
            }

            bool operator == (TYPE4 const &vector) const noexcept
            {
                return (getTuple() == vector.getTuple());
            }

            bool operator != (TYPE4 const &vector) const noexcept
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

            TYPE4 &operator = (TYPE4 const &vector) noexcept
            {
                std::tie(x, y, z, w) = vector.getTuple();
                return (*this);
            }

            void operator -= (TYPE4 const &vector) noexcept
            {
				x -= vector.x;
				y -= vector.y;
				z -= vector.z;
				w -= vector.w;
            }

            void operator += (TYPE4 const &vector) noexcept
            {
				x += vector.x;
				y += vector.y;
				z += vector.z;
				w += vector.w;
			}

            void operator /= (TYPE4 const &vector) noexcept
            {
				x /= vector.x;
				y /= vector.y;
				z /= vector.z;
				w /= vector.w;
			}

            void operator *= (TYPE4 const &vector) noexcept
            {
				x *= vector.x;
				y *= vector.y;
				z *= vector.z;
				w *= vector.w;
			}

            TYPE4 operator - (TYPE4 const &vector) const noexcept
            {
				return TYPE4(
					(x - vector.x),
					(y - vector.y),
					(z - vector.z),
					(w - vector.w));
            }

            TYPE4 operator + (TYPE4 const &vector) const noexcept
            {
				return TYPE4(
					(x + vector.x),
					(y + vector.y),
					(z + vector.z),
					(w + vector.w));
			}

            TYPE4 operator / (TYPE4 const &vector) const noexcept
            {
				return TYPE4(
					(x / vector.x),
					(y / vector.y),
					(z / vector.z),
					(w / vector.w));
			}

            TYPE4 operator * (TYPE4 const &vector) const noexcept
            {
				return TYPE4(
					(x * vector.x),
					(y * vector.y),
					(z * vector.z),
					(w * vector.w));
			}

            // scalar operations
            void operator -= (TYPE scalar) noexcept
            {
				x -= scalar;
				y -= scalar;
				z -= scalar;
				w -= scalar;
            }

            void operator += (TYPE scalar) noexcept
            {
				x += scalar;
				y += scalar;
				z += scalar;
				w += scalar;
			}

            void operator /= (TYPE scalar) noexcept
            {
				x /= scalar;
				y /= scalar;
				z /= scalar;
				w /= scalar;
			}

            void operator *= (TYPE scalar) noexcept
            {
				x *= scalar;
				y *= scalar;
				z *= scalar;
				w *= scalar;
			}

            TYPE4 operator - (TYPE scalar) const noexcept
            {
                return TYPE4(
                    (x - scalar),
                    (y - scalar),
                    (z - scalar),
                    (w - scalar));
            };

            TYPE4 operator + (TYPE scalar) const noexcept
            {
                return TYPE4(
                    (x + scalar),
                    (y + scalar),
                    (z + scalar),
                    (w + scalar));
			}

            TYPE4 operator / (TYPE scalar) const noexcept
            {
                return TYPE4(
                    (x / scalar),
                    (y / scalar),
                    (z / scalar),
                    (w / scalar));
			}

            TYPE4 operator * (TYPE scalar) const noexcept
            {
                return TYPE4(
                    (x * scalar),
                    (y * scalar),
                    (z * scalar),
                    (w * scalar));
			}
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator - (Vector4<TYPE> const &vector) noexcept
        {
			return Vector4<TYPE>(-vector.x, -vector.y, -vector.z, -vector.w);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator + (TYPE scalar, Vector4<TYPE> const &vector) noexcept
        {
			return Vector4<TYPE>(
				(scalar + vector.x),
				(scalar + vector.y),
				(scalar + vector.z),
				(scalar + vector.w));
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator - (TYPE scalar, Vector4<TYPE> const &vector) noexcept
        {
			return Vector4<TYPE>(
				(scalar - vector.x),
				(scalar - vector.y),
				(scalar - vector.z),
				(scalar - vector.w));
		}

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator * (TYPE scalar, Vector4<TYPE> const &vector) noexcept
        {
			return Vector4<TYPE>(
				(scalar * vector.x),
				(scalar * vector.y),
				(scalar * vector.z),
				(scalar * vector.w));
		}

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator / (TYPE scalar, Vector4<TYPE> const &vector) noexcept
        {
			return Vector4<TYPE>(
				(scalar / vector.x),
				(scalar / vector.y),
				(scalar / vector.z),
				(scalar / vector.w));
		}

        using Float4 = Vector4<float>;
        using Int4 = Vector4<int32_t>;
        using UInt4 = Vector4<uint32_t>;
	}; // namespace Math
}; // namespace Gek

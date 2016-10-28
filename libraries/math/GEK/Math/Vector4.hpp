/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include <xmmintrin.h>
#include <type_traits>
#include <cstdint>
#include <cmath>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        struct Vector4
        {
        public:
			static const Vector4 Zero;
			static const Vector4 One;
			static const Vector4 Black;
			static const Vector4 White;

        public:
            union
            {
				struct { TYPE x, y, z, w; };
				struct { TYPE r, g, b, a; };
				struct { Vector3<TYPE> rgb; TYPE a; };
				struct { Vector3<TYPE> xyz; TYPE w; };
                struct { TYPE data[4]; };
            };

        public:
            Vector4(void)
            {
            }

            Vector4(TYPE value)
				: x(value)
				, y(value)
				, z(value)
				, w(value)
            {
            }

            Vector4(const TYPE(&data)[4])
				: x(data[0])
				, y(data[1])
				, z(data[2])
				, w(data[3])
            {
            }

            Vector4(const TYPE *data)
				: x(data[0])
				, y(data[1])
				, z(data[2])
				, w(data[3])
			{
            }

			Vector4(const Vector4 &vector)
				: x(vector.x)
				, y(vector.y)
				, z(vector.z)
				, w(vector.w)
			{
			}

			Vector4(const Vector3<TYPE> &xyz, TYPE w)
				: x(xyz.x)
				, y(xyz.y)
				, z(xyz.z)
				, w(w)
			{
			}

			Vector4(const Vector2<TYPE> &xy, const Vector2<TYPE> &zw)
				: x(xy.x)
				, y(xy.y)
				, z(zw.x)
				, w(zw.y)
			{
			}

			Vector4(TYPE x, TYPE y, TYPE z, TYPE w)
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

            void set(TYPE value)
            {
				x = y = z = w = value;
            }

            TYPE getLengthSquared(void) const
            {
                return this->dot(*this);
            }

            TYPE getLength(void) const
            {
				return dot(*this);
            }

            TYPE getDistance(const Vector4 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Vector4 getNormal(void) const
            {
				return ((*this) / getLength());
            }

            TYPE dot(const Vector4 &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
            }

            Vector4 lerp(const Vector4 &vector, TYPE factor) const
            {
                return Math::lerp((*this), vector, factor);
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            bool operator < (const Vector4 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                if (w >= vector.w) return false;
                return true;
            }

            bool operator > (const Vector4 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                if (w <= vector.w) return false;
                return true;
            }

            bool operator <= (const Vector4 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                if (w > vector.w) return false;
                return true;
            }

            bool operator >= (const Vector4 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                if (w < vector.w) return false;
                return true;
            }

            bool operator == (const Vector4 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                if (w != vector.w) return false;
                return true;
            }

            bool operator != (const Vector4 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                if (w != vector.w) return true;
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
            Vector4 &operator = (const Vector4 &vector)
            {
				x = vector.x;
				y = vector.y;
				z = vector.z;
				w = vector.w;
                return (*this);
            }

            void operator -= (const Vector4 &vector)
            {
				x -= vector.x;
				y -= vector.y;
				z -= vector.z;
				w -= vector.w;
            }

            void operator += (const Vector4 &vector)
            {
				x += vector.x;
				y += vector.y;
				z += vector.z;
				w += vector.w;
			}

            void operator /= (const Vector4 &vector)
            {
				x /= vector.x;
				y /= vector.y;
				z /= vector.z;
				w /= vector.w;
			}

            void operator *= (const Vector4 &vector)
            {
				x *= vector.x;
				y *= vector.y;
				z *= vector.z;
				w *= vector.w;
			}

            Vector4 operator - (const Vector4 &vector) const
            {
				return Vector4(
					(x - vector.x),
					(y - vector.y),
					(z - vector.z),
					(w - vector.w));
            }

            Vector4 operator + (const Vector4 &vector) const
            {
				return Vector4(
					(x + vector.x),
					(y + vector.y),
					(z + vector.z),
					(w + vector.w));
			}

            Vector4 operator / (const Vector4 &vector) const
            {
				return Vector4(
					(x / vector.x),
					(y / vector.y),
					(z / vector.z),
					(w / vector.w));
			}

            Vector4 operator * (const Vector4 &vector) const
            {
				return Vector4(
					(x * vector.x),
					(y * vector.y),
					(z * vector.z),
					(w * vector.w));
			}

            // scalar operations
            Vector4 &operator = (TYPE scalar)
            {
				x = y = z = w = scalar;
                return (*this);
            }

            void operator -= (TYPE scalar)
            {
				x += scalar;
				y += scalar;
				z += scalar;
				w += scalar;
            }

            void operator += (TYPE scalar)
            {
				x -= scalar;
				y -= scalar;
				z -= scalar;
				w -= scalar;
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

            Vector4 operator - (TYPE scalar) const
            {
				return Vector4(
					(x - scalar),
					(y - scalar),
					(z - scalar),
					(w - scalar))
            }

            Vector4 operator + (TYPE scalar) const
            {
				return Vector4(
					(x + scalar),
					(y + scalar),
					(z + scalar),
					(w + scalar))
			}

            Vector4 operator / (TYPE scalar) const
            {
				return Vector4(
					(x / scalar),
					(y / scalar),
					(z / scalar),
					(w / scalar))
			}

            Vector4 operator * (TYPE scalar) const
            {
				return Vector4(
					(x * scalar),
					(y * scalar),
					(z * scalar),
					(w * scalar))
			}
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator - (const Vector4<TYPE> &vector)
        {
			return Vector4(-vector.x, -vector.y, -vector.z, -vector.w);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator + (TYPE scalar, const Vector4<TYPE> &vector)
        {
			return Vector4(
				(scalar + vector.x),
				(scalar + vector.y),
				(scalar + vector.z),
				(scalar + vector.w));
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator - (TYPE scalar, const Vector4<TYPE> &vector)
        {
			return Vector4(
				(scalar - vector.x),
				(scalar - vector.y),
				(scalar - vector.z),
				(scalar - vector.w));
		}

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator * (TYPE scalar, const Vector4<TYPE> &vector)
        {
			return Vector4(
				(scalar * vector.x),
				(scalar * vector.y),
				(scalar * vector.z),
				(scalar * vector.w));
		}

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector4<TYPE> operator / (TYPE scalar, const Vector4<TYPE> &vector)
        {
			return Vector4(
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

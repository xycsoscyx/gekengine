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
				struct { Vector2<TYPE> xy; Vector2<TYPE> zw; };
				struct { Vector2<TYPE> minimum; Vector2<TYPE> maximum; };
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

            Vector4(TYPE x, TYPE y, TYPE z, TYPE w)
				: x(x)
				, y(y)
				, z(z)
				, w(w)
			{
            }

            Vector4(const TYPE *data)
				: x(data[0])
				, y(data[1])
				, z(data[2])
				, w(data[3])
			{
            }

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector4(const Vector4<OTHER> &vector)
				: x(TYPE(vector.x))
				, y(TYPE(vector.y))
				, z(TYPE(vector.z))
				, w(TYPE(vector.w))
			{
			}

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector4(const Vector3<OTHER> &xyz, OTHER w)
				: x(TYPE(xyz.x))
				, y(TYPE(xyz.y))
				, z(TYPE(xyz.z))
				, w(TYPE(w))
			{
			}

            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector4(const Vector2<OTHER> &xy, const Vector2<OTHER> &zw)
				: x(TYPE(xy.x))
				, y(TYPE(xy.y))
				, z(TYPE(zw.x))
				, w(TYPE(zw.y))
			{
			}

            void set(TYPE value)
            {
                x = y = z = w = value;
            }

            void set(TYPE x, TYPE y, TYPE z, TYPE w)
            {
				this->x = x;
				this->y = y;
				this->z = z;
				this->w = w;
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

			Vector4 getMinimum(const Vector4 &vector) const
			{
				return Vector4(
					std::min(x, vector.x),
					std::min(y, vector.y),
					std::min(z, vector.z),
					std::min(w, vector.w)
				);
			}

			Vector4 getMaximum(const Vector4 &vector) const
			{
				return Vector4(
					std::max(x, vector.x),
					std::max(y, vector.y),
					std::max(z, vector.z),
					std::max(w, vector.w)
				);
			}

			Vector4 getClamped(const Vector4 &min, const Vector4 &max) const
			{
				return Vector3(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y),
					std::min(std::max(z, min.z), max.z),
					std::min(std::max(w, min.w), max.w)
				);
			}

			Vector4 getSaturated(void) const
			{
				return getClamped(Zero, One);
			}

			TYPE dot(const Vector4 &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
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
            template <typename OTHER, typename = typename std::enable_if<std::is_arithmetic<OTHER>::value, OTHER>::type>
            Vector4 &operator = (const Vector4<OTHER> &vector)
            {
                x = TYPE(vector.x);
                y = TYPE(vector.y);
                z = TYPE(vector.z);
                w = TYPE(vector.w);
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

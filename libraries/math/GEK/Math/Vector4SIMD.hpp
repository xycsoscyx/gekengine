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
		namespace SIMD
		{
			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			struct Vector4
			{
			public:
				static const Vector4 Zero;
                static const Vector4 Half;
                static const Vector4 One;
                static const Vector4 Two;

			public:
				union
				{
					struct { TYPE x, y, z, w; };
					struct { Vector3<TYPE> xyz; TYPE w; };
					struct { Vector2<TYPE> xy; Vector2<TYPE> zw; };
					struct { Vector2<TYPE> minimum; Vector2<TYPE> maximum; };
					struct { TYPE data[4]; };
					struct { __m128 simd; };
				};

			public:
				Vector4(void)
				{
				}

				Vector4(TYPE value)
					: simd(_mm_set1_ps(value))
				{
				}

				Vector4(__m128 simd)
					: simd(simd)
				{
				}

				Vector4(const TYPE(&data)[4])
					: simd(_mm_loadu_ps(data))
				{
				}

				Vector4(const TYPE *data)
					: simd(_mm_loadu_ps(data))
				{
				}

				Vector4(const Vector4 &vector)
					: simd(vector.simd)
				{
				}

				Vector4(const Vector3<TYPE> &xyz, TYPE w)
					: simd(_mm_setr_ps(xyz.x, xyz.y, xyz.z, w))
				{
				}

				Vector4(const Vector2<TYPE> &xy, const Vector2<TYPE> &zw)
					: simd(_mm_setr_ps(xy.x, xy.y, zw.x, zw.y))
				{
				}

				Vector4(TYPE x, TYPE y, TYPE z, TYPE w)
					: simd(_mm_setr_ps(x, y, z, w))
				{
				}

				void set(TYPE x, TYPE y, TYPE z, TYPE w)
				{
					simd = _mm_setr_ps(x, y, z, w);
				}

				void set(TYPE value)
				{
					this->x = value;
					this->y = value;
					this->z = value;
					this->w = value;
				}

				TYPE getLengthSquared(void) const
				{
					return this->dot(*this);
				}

				TYPE getLength(void) const
				{
					return std::sqrt(getLengthSquared());
				}

				TYPE getDistance(const Vector4 &vector) const
				{
					return (vector - (*this)).getLength();
				}

				Vector4 getNormal(void) const
				{
					return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
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
					return Vector4(
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
					simd = vector.simd;
					return (*this);
				}

				void operator -= (const Vector4 &vector)
				{
					simd = _mm_sub_ps(simd, vector.simd);
				}

				void operator += (const Vector4 &vector)
				{
					simd = _mm_add_ps(simd, vector.simd);
				}

				void operator /= (const Vector4 &vector)
				{
					simd = _mm_div_ps(simd, vector.simd);
				}

				void operator *= (const Vector4 &vector)
				{
					simd = _mm_mul_ps(simd, vector.simd);
				}

				Vector4 operator - (const Vector4 &vector) const
				{
					return _mm_sub_ps(simd, vector.simd);
				}

				Vector4 operator + (const Vector4 &vector) const
				{
					return _mm_add_ps(simd, vector.simd);
				}

				Vector4 operator / (const Vector4 &vector) const
				{
					return _mm_div_ps(simd, vector.simd);
				}

				Vector4 operator * (const Vector4 &vector) const
				{
					return _mm_mul_ps(simd, vector.simd);
				}

				// scalar operations
				Vector4 &operator = (TYPE scalar)
				{
					simd = _mm_set1_ps(scalar);
					return (*this);
				}

				void operator -= (TYPE scalar)
				{
					simd = _mm_sub_ps(simd, _mm_set1_ps(scalar));
				}

				void operator += (TYPE scalar)
				{
					simd = _mm_add_ps(simd, _mm_set1_ps(scalar));
				}

				void operator /= (TYPE scalar)
				{
					simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
				}

				void operator *= (TYPE scalar)
				{
					simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
				}

				Vector4 operator - (TYPE scalar) const
				{
					return _mm_sub_ps(simd, _mm_set1_ps(scalar));
				}

				Vector4 operator + (TYPE scalar) const
				{
					return _mm_add_ps(simd, _mm_set1_ps(scalar));
				}

				Vector4 operator / (TYPE scalar) const
				{
					return _mm_div_ps(simd, _mm_set1_ps(scalar));
				}

				Vector4 operator * (TYPE scalar) const
				{
					return _mm_mul_ps(simd, _mm_set1_ps(scalar));
				}
			};

			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			Vector4<TYPE> operator - (const Vector4<TYPE> &vector)
			{
				return _mm_sub_ps(_mm_set1_ps(0.0f), vector.simd);
			}

			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			Vector4<TYPE> operator + (TYPE scalar, const Vector4<TYPE> &vector)
			{
				return _mm_add_ps(_mm_set1_ps(scalar), vector.simd);
			}

			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			Vector4<TYPE> operator - (TYPE scalar, const Vector4<TYPE> &vector)
			{
				return _mm_sub_ps(_mm_set1_ps(scalar), vector.simd);
			}

			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			Vector4<TYPE> operator * (TYPE scalar, const Vector4<TYPE> &vector)
			{
				return _mm_mul_ps(_mm_set1_ps(scalar), vector.simd);
			}

			template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
			Vector4<TYPE> operator / (TYPE scalar, const Vector4<TYPE> &vector)
			{
				return _mm_div_ps(_mm_set1_ps(scalar), vector.simd);
			}

			using Float4 = Vector4<float>;
		}; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

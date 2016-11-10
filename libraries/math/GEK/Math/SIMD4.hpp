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
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace Gek
{
    namespace Math
    {
		struct SIMD4
		{
		public:
			static const SIMD4 Zero;
            static const SIMD4 Half;
            static const SIMD4 One;
            static const SIMD4 Two;

		public:
			union
			{
				struct { float x, y, z, w; };
				struct { Vector3<float> xyz; float w; };
				struct { Vector2<float> xy; Vector2<float> zw; };
				struct { Vector2<float> minimum; Vector2<float> maximum; };
				struct { float data[4]; };
				struct { __m128 simd; };
			};

		public:
			SIMD4(void)
			{
			}

            SIMD4(float value)
				: simd(_mm_set1_ps(value))
			{
			}

            SIMD4(__m128 simd)
                : simd(simd)
            {
            }

            SIMD4(float x, float y, float z, float w)
				: simd(_mm_setr_ps(x, y, z, w))
			{
			}

            SIMD4(const float *data)
				: simd(_mm_loadu_ps(data))
			{
			}

            SIMD4(const SIMD4 &vector)
				: simd(vector.simd)
			{
			}

            SIMD4(const Vector3<float> &xyz, float w)
				: simd(_mm_setr_ps(xyz.x, xyz.y, xyz.z, w))
			{
			}

            SIMD4(const Vector2<float> &xy, const Vector2<float> &zw)
				: simd(_mm_setr_ps(xy.x, xy.y, zw.x, zw.y))
			{
			}

            void set(float value)
            {
                simd = _mm_set1_ps(value);
            }

            void set(float x, float y, float z, float w)
			{
				simd = _mm_setr_ps(x, y, z, w);
			}

			float getLengthSquared(void) const
			{
				return this->dot(*this);
			}

			float getLength(void) const
			{
				return std::sqrt(getLengthSquared());
			}

			float getDistance(const SIMD4 &vector) const
			{
				return (vector - (*this)).getLength();
			}

			SIMD4 getNormal(void) const
			{
				return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
			}

			SIMD4 getMinimum(const SIMD4 &vector) const
			{
				return SIMD4(
					std::min(x, vector.x),
					std::min(y, vector.y),
					std::min(z, vector.z),
					std::min(w, vector.w)
				);
			}

			SIMD4 getMaximum(const SIMD4 &vector) const
			{
				return SIMD4(
					std::max(x, vector.x),
					std::max(y, vector.y),
					std::max(z, vector.z),
					std::max(w, vector.w)
				);
			}

			SIMD4 getClamped(const SIMD4 &min, const SIMD4 &max) const
			{
				return SIMD4(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y),
					std::min(std::max(z, min.z), max.z),
					std::min(std::max(w, min.w), max.w)
				);
			}

			SIMD4 getSaturated(void) const
			{
				return getClamped(Zero, One);
			}

			float dot(const SIMD4 &vector) const
			{
				return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
			}

			void normalize(void)
			{
				(*this) = getNormal();
			}

			bool operator < (const SIMD4 &vector) const
			{
				if (x >= vector.x) return false;
				if (y >= vector.y) return false;
				if (z >= vector.z) return false;
				if (w >= vector.w) return false;
				return true;
			}

			bool operator > (const SIMD4 &vector) const
			{
				if (x <= vector.x) return false;
				if (y <= vector.y) return false;
				if (z <= vector.z) return false;
				if (w <= vector.w) return false;
				return true;
			}

			bool operator <= (const SIMD4 &vector) const
			{
				if (x > vector.x) return false;
				if (y > vector.y) return false;
				if (z > vector.z) return false;
				if (w > vector.w) return false;
				return true;
			}

			bool operator >= (const SIMD4 &vector) const
			{
				if (x < vector.x) return false;
				if (y < vector.y) return false;
				if (z < vector.z) return false;
				if (w < vector.w) return false;
				return true;
			}

			bool operator == (const SIMD4 &vector) const
			{
				if (x != vector.x) return false;
				if (y != vector.y) return false;
				if (z != vector.z) return false;
				if (w != vector.w) return false;
				return true;
			}

			bool operator != (const SIMD4 &vector) const
			{
				if (x != vector.x) return true;
				if (y != vector.y) return true;
				if (z != vector.z) return true;
				if (w != vector.w) return true;
				return false;
			}

			float operator [] (int axis) const
			{
				return data[axis];
			}

			float &operator [] (int axis)
			{
				return data[axis];
			}

			operator const float *() const
			{
				return data;
			}

			operator float *()
			{
				return data;
			}

			// vector operations
			SIMD4 &operator = (const SIMD4 &vector)
			{
				simd = vector.simd;
				return (*this);
			}

			void operator -= (const SIMD4 &vector)
			{
				simd = _mm_sub_ps(simd, vector.simd);
			}

			void operator += (const SIMD4 &vector)
			{
				simd = _mm_add_ps(simd, vector.simd);
			}

			void operator /= (const SIMD4 &vector)
			{
				simd = _mm_div_ps(simd, vector.simd);
			}

			void operator *= (const SIMD4 &vector)
			{
				simd = _mm_mul_ps(simd, vector.simd);
			}

			SIMD4 operator - (const SIMD4 &vector) const
			{
				return _mm_sub_ps(simd, vector.simd);
			}

			SIMD4 operator + (const SIMD4 &vector) const
			{
				return _mm_add_ps(simd, vector.simd);
			}

			SIMD4 operator / (const SIMD4 &vector) const
			{
				return _mm_div_ps(simd, vector.simd);
			}

			SIMD4 operator * (const SIMD4 &vector) const
			{
				return _mm_mul_ps(simd, vector.simd);
			}

			// scalar operations
			void operator -= (float scalar)
			{
				simd = _mm_sub_ps(simd, _mm_set1_ps(scalar));
			}

			void operator += (float scalar)
			{
				simd = _mm_add_ps(simd, _mm_set1_ps(scalar));
			}

			void operator /= (float scalar)
			{
				simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
			}

			void operator *= (float scalar)
			{
				simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
			}

			SIMD4 operator - (float scalar) const
			{
				return _mm_sub_ps(simd, _mm_set1_ps(scalar));
			}

			SIMD4 operator + (float scalar) const
			{
				return _mm_add_ps(simd, _mm_set1_ps(scalar));
			}

			SIMD4 operator / (float scalar) const
			{
				return _mm_div_ps(simd, _mm_set1_ps(scalar));
			}

			SIMD4 operator * (float scalar) const
			{
				return _mm_mul_ps(simd, _mm_set1_ps(scalar));
			}
		};

		SIMD4 operator - (const SIMD4 &vector)
		{
			return _mm_sub_ps(_mm_set1_ps(0.0f), vector.simd);
		}

		SIMD4 operator + (float scalar, const SIMD4 &vector)
		{
			return _mm_add_ps(_mm_set1_ps(scalar), vector.simd);
		}

		SIMD4 operator - (float scalar, const SIMD4 &vector)
		{
			return _mm_sub_ps(_mm_set1_ps(scalar), vector.simd);
		}

		SIMD4 operator * (float scalar, const SIMD4 &vector)
		{
			return _mm_mul_ps(_mm_set1_ps(scalar), vector.simd);
		}

		SIMD4 operator / (float scalar, const SIMD4 &vector)
		{
			return _mm_div_ps(_mm_set1_ps(scalar), vector.simd);
		}
    }; // namespace Math
}; // namespace Gek

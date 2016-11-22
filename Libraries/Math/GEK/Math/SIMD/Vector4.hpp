/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"
#include <xmmintrin.h>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace Gek
{
    namespace Math
    {
		namespace SIMD
		{
            __declspec(align(16))
			struct Float4
			{
			public:
				static const Float4 Zero;
                static const Float4 Half;
                static const Float4 One;
                static const Float4 Two;

			public:
				union
				{
                    struct { __m128 simd; };
                    const struct { float x, y, z, w; };
                    const struct { Float3 xyz; float w; };
                    const struct { Float2 xy; Float2 zw; };
                    const struct { Float2 minimum; Float2 maximum; };
                    const struct { float data[4]; };
				};

			public:
				inline Float4(void)
				{
				}

                inline Float4(float value)
					: simd(_mm_set1_ps(value))
				{
				}

                inline Float4(__m128 simd)
                    : simd(simd)
                {
                }

                inline Float4(float x, float y, float z, float w)
					: simd(_mm_setr_ps(x, y, z, w))
				{
				}

                inline Float4(const float *data)
					: simd(_mm_loadu_ps(data))
				{
				}

                inline Float4(const Float4 &vector)
                    : simd(vector.simd)
                {
                }

                inline Float4(const Math::Float4 &vector)
                    : simd(_mm_loadu_ps(vector.data))
                {
                }

                inline Float4(const Float3 &xyz, float w)
					: simd(_mm_setr_ps(xyz.x, xyz.y, xyz.z, w))
				{
				}

                inline Float4(const Float2 &xy, const Float2 &zw)
					: simd(_mm_setr_ps(xy.x, xy.y, zw.x, zw.y))
				{
				}

                inline void set(float value)
                {
                    simd = _mm_set1_ps(value);
                }

                inline void set(float x, float y, float z, float w)
				{
					simd = _mm_setr_ps(x, y, z, w);
				}

                inline float getLengthSquared(void) const
				{
					return this->dot(*this);
				}

                inline float getLength(void) const
				{
					return std::sqrt(getLengthSquared());
				}

                inline float getDistance(const Float4 &vector) const
				{
					return (vector - (*this)).getLength();
				}

                inline Float4 getNormal(void) const
				{
					return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
				}

                inline Float4 getMinimum(const Float4 &vector) const
				{
					return Float4(
						std::min(x, vector.x),
						std::min(y, vector.y),
						std::min(z, vector.z),
						std::min(w, vector.w)
					);
				}

                inline Float4 getMaximum(const Float4 &vector) const
				{
					return Float4(
						std::max(x, vector.x),
						std::max(y, vector.y),
						std::max(z, vector.z),
						std::max(w, vector.w)
					);
				}

                inline Float4 getClamped(const Float4 &min, const Float4 &max) const
				{
					return Float4(
						std::min(std::max(x, min.x), max.x),
						std::min(std::max(y, min.y), max.y),
						std::min(std::max(z, min.z), max.z),
						std::min(std::max(w, min.w), max.w)
					);
				}

                inline Float4 getSaturated(void) const
				{
                    return getClamped(Zero, One);
				}

                inline float dot(const Float4 &vector) const
				{
					return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
				}

                inline void normalize(void)
				{
					(*this) = getNormal();
				}

                inline bool operator < (const Float4 &vector) const
				{
					if (x >= vector.x) return false;
					if (y >= vector.y) return false;
					if (z >= vector.z) return false;
					if (w >= vector.w) return false;
					return true;
				}

                inline bool operator > (const Float4 &vector) const
				{
					if (x <= vector.x) return false;
					if (y <= vector.y) return false;
					if (z <= vector.z) return false;
					if (w <= vector.w) return false;
					return true;
				}

                inline bool operator <= (const Float4 &vector) const
				{
					if (x > vector.x) return false;
					if (y > vector.y) return false;
					if (z > vector.z) return false;
					if (w > vector.w) return false;
					return true;
				}

                inline bool operator >= (const Float4 &vector) const
				{
					if (x < vector.x) return false;
					if (y < vector.y) return false;
					if (z < vector.z) return false;
					if (w < vector.w) return false;
					return true;
				}

                inline bool operator == (const Float4 &vector) const
				{
					if (x != vector.x) return false;
					if (y != vector.y) return false;
					if (z != vector.z) return false;
					if (w != vector.w) return false;
					return true;
				}

                inline bool operator != (const Float4 &vector) const
				{
					if (x != vector.x) return true;
					if (y != vector.y) return true;
					if (z != vector.z) return true;
					if (w != vector.w) return true;
					return false;
				}

                inline float operator [] (int axis) const
				{
					return data[axis];
				}

                inline float &operator [] (int axis)
				{
					return data[axis];
				}

                inline operator const __m128 () const
                {
                    return simd;
                }

                inline operator const float *() const
                {
                    return data;
                }

				// vector operations
                inline Float4 &operator = (const Float4 &vector)
                {
                    simd = vector.simd;
                    return (*this);
                }

                inline Float4 &operator = (const Math::Float4 &vector)
                {
                    simd = _mm_loadu_ps(vector.data);
                    return (*this);
                }

                inline Float4 &operator = (const Float3 &vector)
                {
                    simd = _mm_setr_ps(vector.x, vector.y, vector.z, w);
                    return (*this);
                }

                inline void operator -= (const Float4 &vector)
				{
					simd = _mm_sub_ps(simd, vector.simd);
				}

                inline void operator += (const Float4 &vector)
				{
					simd = _mm_add_ps(simd, vector.simd);
				}

                inline void operator /= (const Float4 &vector)
				{
					simd = _mm_div_ps(simd, vector.simd);
				}

                inline void operator *= (const Float4 &vector)
				{
					simd = _mm_mul_ps(simd, vector.simd);
				}

                inline Float4 operator - (const Float4 &vector) const
				{
					return _mm_sub_ps(simd, vector.simd);
				}

                inline Float4 operator + (const Float4 &vector) const
				{
					return _mm_add_ps(simd, vector.simd);
				}

                inline Float4 operator / (const Float4 &vector) const
				{
					return _mm_div_ps(simd, vector.simd);
				}

                inline Float4 operator * (const Float4 &vector) const
				{
					return _mm_mul_ps(simd, vector.simd);
				}

				// scalar operations
                inline void operator -= (float scalar)
				{
					simd = _mm_sub_ps(simd, _mm_set1_ps(scalar));
				}

                inline void operator += (float scalar)
				{
					simd = _mm_add_ps(simd, _mm_set1_ps(scalar));
				}

                inline void operator /= (float scalar)
				{
					simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
				}

                inline void operator *= (float scalar)
				{
					simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
				}

                inline Float4 operator - (float scalar) const
				{
					return _mm_sub_ps(simd, _mm_set1_ps(scalar));
				}

                inline Float4 operator + (float scalar) const
				{
					return _mm_add_ps(simd, _mm_set1_ps(scalar));
				}

                inline Float4 operator / (float scalar) const
				{
					return _mm_div_ps(simd, _mm_set1_ps(scalar));
				}

                inline Float4 operator * (float scalar) const
				{
					return _mm_mul_ps(simd, _mm_set1_ps(scalar));
				}
			};

            inline Float4 operator - (const Float4 &vector)
			{
				return _mm_sub_ps(_mm_set1_ps(0.0f), vector.simd);
			}

            inline Float4 operator + (float scalar, const Float4 &vector)
			{
				return _mm_add_ps(_mm_set1_ps(scalar), vector.simd);
			}

            inline Float4 operator - (float scalar, const Float4 &vector)
			{
				return _mm_sub_ps(_mm_set1_ps(scalar), vector.simd);
			}

            inline Float4 operator * (float scalar, const Float4 &vector)
			{
				return _mm_mul_ps(_mm_set1_ps(scalar), vector.simd);
			}

            inline Float4 operator / (float scalar, const Float4 &vector)
			{
				return _mm_div_ps(_mm_set1_ps(scalar), vector.simd);
			}
		}; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

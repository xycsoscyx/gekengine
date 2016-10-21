/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Vector3.hpp"
#include <xmmintrin.h>

namespace Gek
{
    namespace Math
    {
        struct Quaternion
        {
        public:
            static const Quaternion Identity;

        public:
            union
            {
                struct { float x, y, z, w; };
                struct { float data[4]; };
                struct { __m128 simd; };
                struct { Float3 vector; float w; };
            };

        public:
            Quaternion(void)
            {
            }

            Quaternion(__m128 simd)
                : simd(simd)
            {
            }

            Quaternion(const float(&data)[4])
                : simd(_mm_loadu_ps(data))
            {
            }

            Quaternion(const float *data)
                : simd(_mm_loadu_ps(data))
            {
            }

            Quaternion(const Quaternion &vector)
                : simd(vector.simd)
            {
            }

            Quaternion(float x, float y, float z, float w)
                : data{ x, y, z, w }
            {
            }

            Quaternion &set(float x, float y, float z, float w)
            {
                simd = _mm_setr_ps(x, y, z, w);
                return (*this);
            }

            static Quaternion createEulerRotation(float pitch, float yaw, float roll);
            static Quaternion createAngularRotation(const Float3 &axis, float radians);

            float getLengthSquared(void) const;
            float getLength(void) const;
            Quaternion getNormal(void) const;
            Quaternion getInverse(void) const;

            void invert(void);
            void normalize(void);
            float dot(const Quaternion &rotation) const;
            Quaternion slerp(const Quaternion &rotation, float factor) const;

            bool operator == (const Quaternion &vector) const;
            bool operator != (const Quaternion &vector) const;

            operator const float *() const
            {
                return data;
            }

            operator float *()
            {
                return data;
            }

            Float3 operator * (const Float3 &vector) const;
            Quaternion operator * (const Quaternion &rotation) const;
            void operator *= (const Quaternion &rotation);
            Quaternion &operator = (const Quaternion &rotation);

            void operator /= (float scalar)
            {
                simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            void operator *= (float scalar)
            {
                simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator / (float scalar) const
            {
                return _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator + (float scalar) const
            {
                return _mm_add_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator * (float scalar) const
            {
                return _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator + (const Quaternion &rotation) const
            {
                return _mm_add_ps(simd, rotation.simd);
            }
        };
    }; // namespace Math
}; // namespace Gek

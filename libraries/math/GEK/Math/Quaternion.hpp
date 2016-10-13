/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Math\Float4.hpp"

namespace Gek
{
    namespace Math
    {
        struct Float4x4;

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
            inline Quaternion(void)
            {
            }

            inline Quaternion(__m128 simd)
                : simd(simd)
            {
            }

            inline Quaternion(const float(&data)[4])
                : simd(_mm_loadu_ps(data))
            {
            }

            inline Quaternion(const float *data)
                : simd(_mm_loadu_ps(data))
            {
            }

            inline Quaternion(const Quaternion &vector)
                : simd(vector.simd)
            {
            }

            inline Quaternion(float x, float y, float z, float w)
                : data{ x, y, z, w }
            {
            }

            inline Quaternion &set(float x, float y, float z, float w)
            {
                simd = _mm_setr_ps(x, y, z, w);
                return (*this);
            }

            static Quaternion createEulerRotation(float pitch, float yaw, float roll);
            static Quaternion createAngularRotation(const Float3 &axis, float radians);

            Float4x4 getMatrix(void) const;

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

            inline operator const float *() const
            {
                return data;
            }

            inline operator float *()
            {
                return data;
            }

            Float3 operator * (const Float3 &vector) const;
            Quaternion operator * (const Quaternion &rotation) const;
            void operator *= (const Quaternion &rotation);
            Quaternion &operator = (const Quaternion &rotation);

            inline void operator /= (float scalar)
            {
                simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            inline void operator *= (float scalar)
            {
                simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            inline Quaternion operator / (float scalar) const
            {
                return _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            inline Quaternion operator + (float scalar) const
            {
                return _mm_add_ps(simd, _mm_set1_ps(scalar));
            }

            inline Quaternion operator * (float scalar) const
            {
                return _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            inline Quaternion operator + (const Quaternion &rotation) const
            {
                return _mm_add_ps(simd, rotation.simd);
            }
        };
    }; // namespace Math
}; // namespace Gek

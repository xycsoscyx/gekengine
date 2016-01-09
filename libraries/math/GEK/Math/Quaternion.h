#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Math
    {
        struct Float4x4;

        struct Quaternion
        {
        public:
            union
            {
                struct { float x, y, z, w; };
                struct { float data[4]; };
                struct { __m128 simd; };
            };

        public:
            Quaternion(void)
                : data{ 0.0f, 0.0f, 0.0f, 1.0f }
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
                normalize();
            }

            Quaternion(float pitch, float yaw, float roll)
            {
                setEulerRotation(pitch, yaw, roll);
            }

            Quaternion(const Float3 &axis, float radians)
            {
                setAngularRotation(axis, radians);
            }

            inline void set(float x, float y, float z, float w)
            {
                simd = _mm_setr_ps(x, y, z, w);
            }

            inline void set(const Quaternion &vector)
            {
                simd = vector.simd;
            }

            void setEulerRotation(float pitch, float yaw, float roll);
            void setAngularRotation(const Float3 &axis, float radians);

            Float4x4 getMatrix(const Float3 &translation = Float3(0.0f, 0.0f, 0.0f)) const;

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
            Quaternion operator = (const Quaternion &rotation);

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

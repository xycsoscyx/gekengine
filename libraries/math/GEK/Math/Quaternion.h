#pragma once

#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Math
    {
        struct Float3;
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

            Quaternion(const Float4x4 &rotation);
            Quaternion(const Float3 &axis, float radians);
            Quaternion(const Float3 &euler);
            Quaternion(float x, float y, float z);

            inline void set(float x, float y, float z, float w)
            {
                simd = _mm_setr_ps(x, y, z, w);
            }

            inline void set(const Quaternion &vector)
            {
                simd = vector.simd;
            }

            void setIdentity(void);
            void setEuler(const Float3 &euler);
            void setEuler(float x, float y, float z);
            void setRotation(const Float3 &axis, float radians);
            void setRotation(const Float4x4 &rotation);

            float getLengthSquared(void) const;
            float getLength(void) const;
            Quaternion getNormal(void) const;
            Quaternion getInverse(void) const;
            Float3 getEuler(void) const;

            void invert(void);
            void normalize(void);
            float dot(const Quaternion &rotation) const;
            Quaternion lerp(const Quaternion &rotation, float factor) const;
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
            Quaternion operator = (const Float4x4 &rotation);
        };
    }; // namespace Math
}; // namespace Gek

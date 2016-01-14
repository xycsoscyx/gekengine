#pragma once

#include <xmmintrin.h>

namespace Gek
{
    namespace Math
    {
        struct Float3;

        struct Float4
        {
        public:
            union
            {
                struct { float x, y, z, w; };
                struct { float r, g, b, a; };
                struct { float data[4]; };
                struct { __m128 simd; };
            };

        public:
            Float4(void)
                : simd(_mm_set1_ps(0.0f))
            {
            }

            Float4(float value)
                : simd(_mm_set1_ps(value))
            {
            }

            Float4(__m128 simd)
                : simd(simd)
            {
            }

            Float4(const float(&data)[4])
                : simd(_mm_loadu_ps(data))
            {
            }

            Float4(const float *data)
                : simd(_mm_loadu_ps(data))
            {
            }

            Float4(const Float4 &vector)
                : simd(vector.simd)
            {
            }

            Float4(float x, float y, float z, float w)
                : simd(_mm_setr_ps(x, y, z, w))
            {
            }

            Float3 getXYZ(void) const;
            __declspec(property(get = getXYZ)) Float3 xyz;

            inline void set(float value)
            {
                simd = _mm_set1_ps(value);
            }

            inline void set(float x, float y, float z, float w)
            {
                simd = _mm_setr_ps(x, y, z, w);
            }

            inline void set(const Float4 &vector)
            {
                simd = vector.simd;
            }

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getDistance(const Float4 &vector) const;
            Float4 getNormal(void) const;

            float dot(const Float4 &vector) const;
            Float4 lerp(const Float4 &vector, float factor) const;
            void normalize(void);

            bool operator < (const Float4 &vector) const;
            bool operator > (const Float4 &vector) const;
            bool operator <= (const Float4 &vector) const;
            bool operator >= (const Float4 &vector) const;
            bool operator == (const Float4 &vector) const;
            bool operator != (const Float4 &vector) const;

            inline float operator [] (int axis) const
            {
                return data[axis];
            }

            inline float &operator [] (int axis)
            {
                return data[axis];
            }

            inline operator const float *() const
            {
                return data;
            }

            inline operator float *()
            {
                return data;
            }

            // vector operations
            inline Float4 &operator = (const Float4 &vector)
            {
                simd = vector.simd;
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
            inline Float4 &operator = (float scalar)
            {
                simd = _mm_set1_ps(scalar);
                return (*this);
            }

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
    }; // namespace Math
}; // namespace Gek

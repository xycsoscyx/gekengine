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
                : data{ 0.0f, 0.0f, 0.0f, 0.0f }
            {
            }

            Float4(float value)
                : data{ value, value, value, value }
            {
            }

            Float4(__m128 simd)
                : simd(simd)
            {
            }

            Float4(const float(&data)[4])
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            Float4(const float *data)
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            Float4(const Float4 &vector)
                : simd(vector.simd)
            {
            }

            Float4(float x, float y, float z, float w)
                : data{ x, y, z, w }
            {
            }

            Float3 getXYZ(void) const;
            __declspec(property(get = getXYZ)) Float3 xyz;

            void set(float value);
            void set(float x, float y, float z, float w);
            void set(const Float4 &vector);

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getDistance(const Float4 &vector) const;
            Float4 getNormal(void) const;

            float dot(const Float4 &vector) const;
            Float4 lerp(const Float4 &vector, float factor) const;
            void normalize(void);

            float operator [] (int axis) const;
            float &operator [] (int axis);

            operator const float *() const;
            operator float *();

            bool operator < (const Float4 &vector) const;
            bool operator > (const Float4 &vector) const;
            bool operator <= (const Float4 &vector) const;
            bool operator >= (const Float4 &vector) const;
            bool operator == (const Float4 &vector) const;
            bool operator != (const Float4 &vector) const;

            Float4 operator = (const Float4 &vector);
            void operator -= (const Float4 &vector);
            void operator += (const Float4 &vector);
            void operator /= (const Float4 &vector);
            void operator *= (const Float4 &vector);

            Float4 operator - (const Float4 &vector) const;
            Float4 operator + (const Float4 &vector) const;
            Float4 operator / (const Float4 &vector) const;
            Float4 operator * (const Float4 &vector) const;
        };

        Float4 operator - (const Float4 &vector);
    }; // namespace Math
}; // namespace Gek

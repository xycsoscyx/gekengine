#pragma once

namespace Gek
{
    namespace Math
    {
        struct Float4;

        struct Float3
        {
        public:
            union
            {
                struct { float x, y, z; };
                struct { float data[3]; };
            };

        public:
            Float3(void)
                : data{ 0.0f, 0.0f, 0.0f }
            {
            }

            Float3(const float(&data)[3])
                : data{ data[0], data[1], data[2] }
            {
            }

            Float3(const float *data)
                : data{ data[0], data[1], data[2] }
            {
            }

            Float3(float scalar)
                : data{ scalar, scalar, scalar }
            {
            }

            Float3(const Float3 &vector)
                : data{ vector.x, vector.y, vector.z }
            {
            }

            Float3(float x, float y, float z)
                : data{ x, y, z }
            {
            }

            Float4 w(float w);

            void set(float value);
            void set(float x, float y, float z);
            void set(const Float3 &vector);

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getDistance(const Float3 &vector) const;
            Float3 getNormal(void) const;

            float dot(const Float3 &vector) const;
            Float3 cross(const Float3 &vector) const;
            Float3 lerp(const Float3 &vector, float factor) const;
            void normalize(void);

            float operator [] (int axis) const;
            float &operator [] (int axis);

            operator const float *() const;
            operator float *();

            bool operator < (const Float3 &vector) const;
            bool operator > (const Float3 &vector) const;
            bool operator <= (const Float3 &vector) const;
            bool operator >= (const Float3 &vector) const;
            bool operator == (const Float3 &vector) const;
            bool operator != (const Float3 &vector) const;

            Float3 operator = (const Float3 &vector);
            void operator -= (const Float3 &vector);
            void operator += (const Float3 &vector);
            void operator /= (const Float3 &vector);
            void operator *= (const Float3 &vector);
            Float3 operator - (const Float3 &vector) const;
            Float3 operator + (const Float3 &vector) const;
            Float3 operator / (const Float3 &vector) const;
            Float3 operator * (const Float3 &vector) const;
        };

        Float3 operator - (const Float3 &vector);
    }; // namespace Math
}; // namespace Gek

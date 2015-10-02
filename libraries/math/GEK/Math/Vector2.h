#pragma once

namespace Gek
{
    namespace Math
    {
        struct Float2
        {
        public:
            union
            {
                struct { float x, y; };
                struct { float u, v; };
                struct { float data[2]; };
            };

        public:
            Float2(void);
            Float2(const float(&data)[2]);
            Float2(const float *data);
            Float2(float scalar);
            Float2(const Float2 &vector);
            Float2(float x, float y);

            void set(float value);
            void set(float x, float y);
            void setLength(float length);

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getMax(void) const;
            float getDistance(const Float2 &vector) const;
            Float2 getNormal(void) const;

            void normalize(void);
            float dot(const Float2 &vector) const;
            Float2 lerp(const Float2 &vector, float factor) const;

            float operator [] (int axis) const;
            float &operator [] (int axis);

            operator const float *() const;
            operator float *();

            bool operator < (const Float2 &vector) const;
            bool operator > (const Float2 &vector) const;
            bool operator <= (const Float2 &vector) const;
            bool operator >= (const Float2 &vector) const;
            bool operator == (const Float2 &vector) const;
            bool operator != (const Float2 &vector) const;

            Float2 operator = (const Float2 &vector);
            void operator -= (const Float2 &vector);
            void operator += (const Float2 &vector);
            void operator /= (const Float2 &vector);
            void operator *= (const Float2 &vector);
            Float2 operator - (const Float2 &vector) const;
            Float2 operator + (const Float2 &vector) const;
            Float2 operator / (const Float2 &vector) const;
            Float2 operator * (const Float2 &vector) const;
        };

        Float2 operator - (const Float2 &vector);
    }; // namespace Math
}; // namespace Gek

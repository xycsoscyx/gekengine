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
            Float3(void);
            Float3(const float(&data)[3]);
            Float3(const float *data);
            Float3(float scalar);
            Float3(const Float3 &vector);
            Float3(float x, float y, float z);

            Float4 w(float w);

            void set(float value);
            void set(float x, float y, float z);
            void setLength(float length);

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getMax(void) const;
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

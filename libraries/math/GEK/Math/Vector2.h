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
            Float2(void)
            {
            }

            Float2(const float(&data)[2])
                : data{ data[0], data[1] }
            {
            }

            Float2(const float *data)
                : data{ data[0], data[1] }
            {
            }

            Float2(float scalar)
                : data{ scalar, scalar }
            {
            }

            Float2(const Float2 &vector)
                : data{ vector.data[0], vector.data[1] }
            {
            }

            Float2(float x, float y)
                : data{ x, y }
            {
            }

            inline void set(float x, float y)
            {
                this->x = x;
                this->y = y;
            }

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getDistance(const Float2 &vector) const;
            Float2 getNormal(void) const;

            void normalize(void);
            float dot(const Float2 &vector) const;
            Float2 lerp(const Float2 &vector, float factor) const;

            bool operator < (const Float2 &vector) const;
            bool operator > (const Float2 &vector) const;
            bool operator <= (const Float2 &vector) const;
            bool operator >= (const Float2 &vector) const;
            bool operator == (const Float2 &vector) const;
            bool operator != (const Float2 &vector) const;

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
            inline Float2 &operator = (const Float2 &vector)
            {
                x = vector.x;
                y = vector.y;
                return (*this);
            }

            inline void operator -= (const Float2 &vector)
            {
                x -= vector.x;
                y -= vector.y;
            }

            inline void operator += (const Float2 &vector)
            {
                x += vector.x;
                y += vector.y;
            }

            inline void operator /= (const Float2 &vector)
            {
                x /= vector.x;
                y /= vector.y;
            }

            inline void operator *= (const Float2 &vector)
            {
                x *= vector.x;
                y *= vector.y;
            }

            inline Float2 operator - (const Float2 &vector) const
            {
                return Float2((x - vector.x), (y - vector.y));
            }

            inline Float2 operator + (const Float2 &vector) const
            {
                return Float2((x + vector.x), (y + vector.y));
            }

            inline Float2 operator / (const Float2 &vector) const
            {
                return Float2((x / vector.x), (y / vector.y));
            }

            inline Float2 operator * (const Float2 &vector) const
            {
                return Float2((x * vector.x), (y * vector.y));
            }

            // scalar operations
            inline Float2 &operator = (float scalar)
            {
                x = scalar;
                y = scalar;
                return (*this);
            }

            inline void operator -= (float scalar)
            {
                x -= scalar;
                y -= scalar;
            }

            inline void operator += (float scalar)
            {
                x += scalar;
                y += scalar;
            }

            inline void operator /= (float scalar)
            {
                x /= scalar;
                y /= scalar;
            }

            inline void operator *= (float scalar)
            {
                x *= scalar;
                y *= scalar;
            }

            inline Float2 operator - (float scalar) const
            {
                return Float2((x - scalar), (y - scalar));
            }

            inline Float2 operator + (float scalar) const
            {
                return Float2((x + scalar), (y + scalar));
            }

            inline Float2 operator / (float scalar) const
            {
                return Float2((x / scalar), (y / scalar));
            }

            inline Float2 operator * (float scalar) const
            {
                return Float2((x * scalar), (y * scalar));
            }
        };

        inline Float2 operator - (const Float2 &vector)
        {
            return Float2(-vector.x, -vector.y);
        }

        inline Float2 operator + (float scalar, const Float2 &vector)
        {
            return Float2(scalar + vector.x, scalar + vector.y);
        }

        inline Float2 operator - (float scalar, const Float2 &vector)
        {
            return Float2(scalar - vector.x, scalar - vector.y);
        }

        inline Float2 operator * (float scalar, const Float2 &vector)
        {
            return Float2(scalar * vector.x, scalar * vector.y);
        }

        inline Float2 operator / (float scalar, const Float2 &vector)
        {
            return Float2(scalar / vector.x, scalar / vector.y);
        }
    }; // namespace Math
}; // namespace Gek

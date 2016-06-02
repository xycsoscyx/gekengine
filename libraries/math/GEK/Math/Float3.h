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
                : data{ vector.data[0], vector.data[1], vector.data[2] }
            {
            }

            Float3(float x, float y, float z)
                : data{ x, y, z }
            {
            }

            Float4 w(float w) const;

            inline void set(float x, float y, float z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            inline void set(float value)
            {
                this->x = value;
                this->y = value;
                this->z = value;
            }

            float getLengthSquared(void) const;
            float getLength(void) const;
            float getDistance(const Float3 &vector) const;
            Float3 getNormal(void) const;

            float dot(const Float3 &vector) const;
            Float3 cross(const Float3 &vector) const;
            Float3 lerp(const Float3 &vector, float factor) const;
            void normalize(void);

            bool operator < (const Float3 &vector) const;
            bool operator > (const Float3 &vector) const;
            bool operator <= (const Float3 &vector) const;
            bool operator >= (const Float3 &vector) const;
            bool operator == (const Float3 &vector) const;
            bool operator != (const Float3 &vector) const;

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
            inline Float3 &operator = (const Float3 &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            inline void operator -= (const Float3 &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            inline void operator += (const Float3 &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            inline void operator /= (const Float3 &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            inline void operator *= (const Float3 &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            inline Float3 operator - (const Float3 &vector) const
            {
                return Float3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            inline Float3 operator + (const Float3 &vector) const
            {
                return Float3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            inline Float3 operator / (const Float3 &vector) const
            {
                return Float3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            inline Float3 operator * (const Float3 &vector) const
            {
                return Float3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            // scalar operations
            inline Float3 &operator = (float scalar)
            {
                x = scalar;
                y = scalar;
                z = scalar;
                return (*this);
            }

            inline void operator -= (float scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            inline void operator += (float scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            inline void operator /= (float scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            inline void operator *= (float scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            inline Float3 operator - (float scalar) const
            {
                return Float3((x - scalar), (y - scalar), (z - scalar));
            }

            inline Float3 operator + (float scalar) const
            {
                return Float3((x + scalar), (y + scalar), (z + scalar));
            }

            inline Float3 operator / (float scalar) const
            {
                return Float3((x / scalar), (y / scalar), (z / scalar));
            }

            inline Float3 operator * (float scalar) const
            {
                return Float3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        inline Float3 operator - (const Float3 &vector)
        {
            return Float3(-vector.x, -vector.y, -vector.z);
        }

        inline Float3 operator + (float scalar, const Float3 &vector)
        {
            return Float3(scalar + vector.x, scalar + vector.y, scalar + vector.z);
        }

        inline Float3 operator - (float scalar, const Float3 &vector)
        {
            return Float3(scalar - vector.x, scalar - vector.y, scalar - vector.z);
        }

        inline Float3 operator * (float scalar, const Float3 &vector)
        {
            return Float3(scalar * vector.x, scalar * vector.y, scalar * vector.z);
        }

        inline Float3 operator / (float scalar, const Float3 &vector)
        {
            return Float3(scalar / vector.x, scalar / vector.y, scalar / vector.z);
        }
    }; // namespace Math
}; // namespace Gek

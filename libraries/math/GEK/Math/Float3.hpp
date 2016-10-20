/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

namespace Gek
{
    namespace Math
    {
        template <typename TYPE>
        struct Vector3
        {
        public:
            static const Vector3 Zero;
            static const Vector3 One;

        public:
            union
            {
                struct { TYPE x, y, z; };
                struct { TYPE data[3]; };
            };

        public:
            inline Vector3(void)
            {
            }

            inline Vector3(const TYPE(&data)[3])
                : data{ data[0], data[1], data[2] }
            {
            }

            inline Vector3(const TYPE *data)
                : data{ data[0], data[1], data[2] }
            {
            }

            inline Vector3(TYPE scalar)
                : data{ scalar, scalar, scalar }
            {
            }

            inline Vector3(const Vector3 &vector)
                : data{ vector.data[0], vector.data[1], vector.data[2] }
            {
            }

            inline Vector3(TYPE x, TYPE y, TYPE z)
                : data{ x, y, z }
            {
            }

            inline void set(TYPE x, TYPE y, TYPE z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            inline void set(TYPE value)
            {
                this->x = value;
                this->y = value;
                this->z = value;
            }

            TYPE getLengthSquared(void) const
            {
                return ((x * x) + (y * y) + (z * z));
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            TYPE getDistance(const Vector3 &vector) const
            {
                return (vector - (*this)).getLength();
            }

            Vector3 getNormal(void) const
            {
                return ((*this) / getLength());
            }

            TYPE dot(const Vector3 &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z));
            }

            Vector3 cross(const Vector3 &vector) const
            {
                return Vector3(((y * vector.z) - (z * vector.y)),
                    ((z * vector.x) - (x * vector.z)),
                    ((x * vector.y) - (y * vector.x)));
            }

            Vector3 lerp(const Vector3 &vector, TYPE factor) const
            {
                return Math::lerp((*this), vector, factor);
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            bool operator < (const Vector3 &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                return true;
            }

            bool operator > (const Vector3 &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                return true;
            }

            bool operator <= (const Vector3 &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                return true;
            }

            bool operator >= (const Vector3 &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                return true;
            }

            bool operator == (const Vector3 &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                return true;
            }

            bool operator != (const Vector3 &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                return false;
            }

            inline TYPE operator [] (int axis) const
            {
                return data[axis];
            }

            inline TYPE &operator [] (int axis)
            {
                return data[axis];
            }

            inline operator const TYPE *() const
            {
                return data;
            }

            inline operator TYPE *()
            {
                return data;
            }

            // vector operations
            inline Vector3 &operator = (const Vector3 &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            inline void operator -= (const Vector3 &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            inline void operator += (const Vector3 &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            inline void operator /= (const Vector3 &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            inline void operator *= (const Vector3 &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            inline Vector3 operator - (const Vector3 &vector) const
            {
                return Vector3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            inline Vector3 operator + (const Vector3 &vector) const
            {
                return Vector3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            inline Vector3 operator / (const Vector3 &vector) const
            {
                return Vector3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            inline Vector3 operator * (const Vector3 &vector) const
            {
                return Vector3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            // scalar operations
            inline Vector3 &operator = (TYPE scalar)
            {
                x = scalar;
                y = scalar;
                z = scalar;
                return (*this);
            }

            inline void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            inline void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            inline void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            inline void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            inline Vector3 operator - (TYPE scalar) const
            {
                return Vector3((x - scalar), (y - scalar), (z - scalar));
            }

            inline Vector3 operator + (TYPE scalar) const
            {
                return Vector3((x + scalar), (y + scalar), (z + scalar));
            }

            inline Vector3 operator / (TYPE scalar) const
            {
                return Vector3((x / scalar), (y / scalar), (z / scalar));
            }

            inline Vector3 operator * (TYPE scalar) const
            {
                return Vector3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        template <typename TYPE>
        inline Vector3<TYPE> operator - (const Vector3<TYPE> &vector)
        {
            return Vector3(-vector.x, -vector.y, -vector.z);
        }

        template <typename TYPE>
        inline Vector3<TYPE> operator + (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3(scalar + vector.x, scalar + vector.y, scalar + vector.z);
        }

        template <typename TYPE>
        inline Vector3<TYPE> operator - (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3(scalar - vector.x, scalar - vector.y, scalar - vector.z);
        }

        template <typename TYPE>
        inline Vector3<TYPE> operator * (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3(scalar * vector.x, scalar * vector.y, scalar * vector.z);
        }

        template <typename TYPE>
        inline Vector3<TYPE> operator / (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3(scalar / vector.x, scalar / vector.y, scalar / vector.z);
        }

        using Float3 = Vector3<float>;
        using Int3 = Vector3<int>;
        using UInt3 = Vector3<unsigned int>;
    }; // namespace Math
}; // namespace Gek

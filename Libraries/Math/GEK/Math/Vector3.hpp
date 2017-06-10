/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector2.hpp"

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        struct Vector3
        {
        public:
            static const Vector3<TYPE> Zero;
            static const Vector3<TYPE> One;

        public:
            union
            {
				struct { TYPE data[3]; };

                struct
                {
                    union
                    {
                        struct { TYPE x, y; };
                        Vector2<TYPE> xy;
                    };
                    
                    TYPE z;
                };
            };

        public:
            Vector3(void)
            {
            }

            Vector3(const Vector3<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
            {
            }

            explicit Vector3(TYPE scalar)
                : data{ scalar, scalar, scalar }
            {
            }

            explicit Vector3(TYPE x, TYPE y, TYPE z)
                : data{ x, y, z }
            {
            }

            explicit Vector3(const TYPE *data)
                : data{ data[0], data[1], data[2] }
            {
            }

            Vector3(const Vector2<TYPE> &xy, TYPE z)
                : x(vector.x)
                , y(vector.y)
                , z(z)
            {
            }

            void set(TYPE value)
            {
                this->x = this->y = this->z = value;
            }

            void set(TYPE x, TYPE y, TYPE z)
            {
                this->x = x;
                this->y = y;
                this->z = z;
            }

            void set(const TYPE *data)
            {
                this->x = data[0];
                this->y = data[1];
                this->z = data[2];
            }

            TYPE getMagnitudeSquared(void) const
            {
                return dot(*this);
            }

            TYPE getMagnitude(void) const
            {
                return std::sqrt(getMagnitudeSquared());
            }

            TYPE getDistance(const Vector3<TYPE> &vector) const
            {
                return (vector - (*this)).getMagnitude();
            }

            Vector3<TYPE> getNormal(void) const
            {
                float inverseMagnitude = (1.0f / getMagnitude());
                return ((*this) * inverseMagnitude);
            }

			Vector3<TYPE> getMinimum(const Vector3<TYPE> &vector) const
			{
				return Vector3(
					std::min(x, vector.x),
					std::min(y, vector.y),
					std::min(z, vector.z)
				);
			}

			Vector3<TYPE> getMaximum(const Vector3<TYPE> &vector) const
			{
				return Vector3(
					std::max(x, vector.x),
					std::max(y, vector.y),
					std::max(z, vector.z)
				);
			}

			Vector3<TYPE> getClamped(const Vector3<TYPE> &min, const Vector3<TYPE> &max) const
			{
				return Vector3(
					std::min(std::max(x, min.x), max.x),
					std::min(std::max(y, min.y), max.y),
					std::min(std::max(z, min.z), max.z)
				);
			}

			Vector3<TYPE> getSaturated(void) const
			{
				return getClamped(Zero, One);
			}

			TYPE dot(const Vector3<TYPE> &vector) const
            {
                return ((x * vector.x) + (y * vector.y) + (z * vector.z));
            }

            Vector3<TYPE> cross(const Vector3<TYPE> &vector) const
            {
                return Vector3(
                    ((y * vector.z) - (z * vector.y)),
                    ((z * vector.x) - (x * vector.z)),
                    ((x * vector.y) - (y * vector.x)));
            }

            void normalize(void)
            {
                float inverseMagnitude = (1.0f / getMagnitude());
                (*this) *= inverseMagnitude;
            }

            bool operator < (const Vector3<TYPE> &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                return true;
            }

            bool operator > (const Vector3<TYPE> &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                return true;
            }

            bool operator <= (const Vector3<TYPE> &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                return true;
            }

            bool operator >= (const Vector3<TYPE> &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                return true;
            }

            bool operator == (const Vector3<TYPE> &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                return true;
            }

            bool operator != (const Vector3<TYPE> &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                return false;
            }

            TYPE operator [] (int axis) const
            {
                return data[axis];
            }

            TYPE &operator [] (int axis)
            {
                return data[axis];
            }

            operator TYPE const *() const
            {
                return data;
            }

            // vector operations
            Vector3<TYPE> &operator = (const Vector3<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                return (*this);
            }

            void operator -= (const Vector3<TYPE> &vector)
            {
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
            }

            void operator += (const Vector3<TYPE> &vector)
            {
                x += vector.x;
                y += vector.y;
                z += vector.z;
            }

            void operator /= (const Vector3<TYPE> &vector)
            {
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
            }

            void operator *= (const Vector3<TYPE> &vector)
            {
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
            }

            Vector3<TYPE> operator - (const Vector3<TYPE> &vector) const
            {
                return Vector3((x - vector.x), (y - vector.y), (z - vector.z));
            }

            Vector3<TYPE> operator + (const Vector3<TYPE> &vector) const
            {
                return Vector3((x + vector.x), (y + vector.y), (z + vector.z));
            }

            Vector3<TYPE> operator / (const Vector3<TYPE> &vector) const
            {
                return Vector3((x / vector.x), (y / vector.y), (z / vector.z));
            }

            Vector3<TYPE> operator * (const Vector3<TYPE> &vector) const
            {
                return Vector3((x * vector.x), (y * vector.y), (z * vector.z));
            }

            // scalar operations
            void operator -= (TYPE scalar)
            {
                x -= scalar;
                y -= scalar;
                z -= scalar;
            }

            void operator += (TYPE scalar)
            {
                x += scalar;
                y += scalar;
                z += scalar;
            }

            void operator /= (TYPE scalar)
            {
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }

            void operator *= (TYPE scalar)
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
            }

            Vector3<TYPE> operator - (TYPE scalar) const
            {
                return Vector3((x - scalar), (y - scalar), (z - scalar));
            }

            Vector3<TYPE> operator + (TYPE scalar) const
            {
                return Vector3((x + scalar), (y + scalar), (z + scalar));
            }

            Vector3<TYPE> operator / (TYPE scalar) const
            {
                return Vector3((x / scalar), (y / scalar), (z / scalar));
            }

            Vector3<TYPE> operator * (TYPE scalar) const
            {
                return Vector3((x * scalar), (y * scalar), (z * scalar));
            }
        };

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator - (const Vector3<TYPE> &vector)
        {
            return Vector3<TYPE>(-vector.x, -vector.y, -vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator + (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3<TYPE>(scalar + vector.x, scalar + vector.y, scalar + vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator - (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3<TYPE>(scalar - vector.x, scalar - vector.y, scalar - vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator * (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3<TYPE>(scalar * vector.x, scalar * vector.y, scalar * vector.z);
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Vector3<TYPE> operator / (TYPE scalar, const Vector3<TYPE> &vector)
        {
            return Vector3<TYPE>(scalar / vector.x, scalar / vector.y, scalar / vector.z);
        }

        using Float3 = Vector3<float>;
        using Int3 = Vector3<int32_t>;
        using UInt3 = Vector3<uint32_t>;
    }; // namespace Math
}; // namespace Gek

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float3::Float3(void)
            : data{ 0.0f, 0.0f, 0.0f }
        {
        }

        Float3::Float3(const float(&data)[3])
            : data{ data[0], data[1], data[2] }
        {
        }

        Float3::Float3(const float *data)
            : data{ data[0], data[1], data[2] }
        {
        }

        Float3::Float3(float scalar)
            : data{ scalar, scalar, scalar }
        {
        }

        Float3::Float3(const Float3 &vector)
            : x(vector.x)
            , y(vector.y)
            , z(vector.z)
        {
        }

        Float3::Float3(float x, float y, float z)
            : x(x)
            , y(y)
            , z(z)
        {
        }

        Float4 Float3::w(float w)
        {
            return Float4(x, y, z, w);
        }

        void Float3::set(float value)
        {
            this->x = this->y = this->z = value;
        }

        void Float3::set(float x, float y, float z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        void Float3::setLength(float length)
        {
            (*this) *= (length / getLength());
        }

        float Float3::getLengthSquared(void) const
        {
            return ((x * x) + (y * y) + (z * z));
        }

        float Float3::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        float Float3::getMax(void) const
        {
            return std::max(std::max(x, y), z);
        }

        float Float3::getDistance(const Float3 &vector) const
        {
            return (vector - (*this)).getLength();
        }

        Float3 Float3::getNormal(void) const
        {
            float length(getLength());
            if (length != 0.0f)
            {
                return ((*this) * (1.0f / length));
            }

            return (*this);
        }

        float Float3::dot(const Float3 &vector) const
        {
            return ((x * vector.x) + (y * vector.y) + (z * vector.z));
        }

        Float3 Float3::cross(const Float3 &vector) const
        {
            return Float3(((y * vector.z) - (z * vector.y)),
                ((z * vector.x) - (x * vector.z)),
                ((x * vector.y) - (y * vector.x)));
        }

        Float3 Float3::lerp(const Float3 &vector, float factor) const
        {
            return Gek::Math::lerp((*this), vector, factor);
        }

        void Float3::normalize(void)
        {
            (*this) = getNormal();
        }

        float Float3::operator [] (int axis) const
        {
            return data[axis];
        }

        float &Float3::operator [] (int axis)
        {
            return data[axis];
        }

        Float3::operator const float *() const
        {
            return data;
        }

        Float3::operator float *()
        {
            return data;
        }

        bool Float3::operator < (const Float3 &vector) const
        {
            if (x >= vector.x) return false;
            if (y >= vector.y) return false;
            if (z >= vector.z) return false;
            return true;
        }

        bool Float3::operator > (const Float3 &vector) const
        {
            if (x <= vector.x) return false;
            if (y <= vector.y) return false;
            if (z <= vector.z) return false;
            return true;
        }

        bool Float3::operator <= (const Float3 &vector) const
        {
            if (x > vector.x) return false;
            if (y > vector.y) return false;
            if (z > vector.z) return false;
            return true;
        }

        bool Float3::operator >= (const Float3 &vector) const
        {
            if (x < vector.x) return false;
            if (y < vector.y) return false;
            if (z < vector.z) return false;
            return true;
        }

        bool Float3::operator == (const Float3 &vector) const
        {
            if (x != vector.x) return false;
            if (y != vector.y) return false;
            if (z != vector.z) return false;
            return true;
        }

        bool Float3::operator != (const Float3 &vector) const
        {
            if (x != vector.x) return true;
            if (y != vector.y) return true;
            if (z != vector.z) return true;
            return false;
        }

        Float3 Float3::operator = (const Float3 &vector)
        {
            x = vector.x;
            y = vector.y;
            z = vector.z;
            return (*this);
        }

        void Float3::operator -= (const Float3 &vector)
        {
            x -= vector.x;
            y -= vector.y;
            z -= vector.z;
        }

        void Float3::operator += (const Float3 &vector)
        {
            x += vector.x;
            y += vector.y;
            z += vector.z;
        }

        void Float3::operator /= (const Float3 &vector)
        {
            x /= vector.x;
            y /= vector.y;
            z /= vector.z;
        }

        void Float3::operator *= (const Float3 &vector)
        {
            x *= vector.x;
            y *= vector.y;
            z *= vector.z;
        }

        Float3 Float3::operator - (const Float3 &vector) const
        {
            return Float3((x - vector.x), (y - vector.y), (z - vector.z));
        }

        Float3 Float3::operator + (const Float3 &vector) const
        {
            return Float3((x + vector.x), (y + vector.y), (z + vector.z));
        }

        Float3 Float3::operator / (const Float3 &vector) const
        {
            return Float3((x / vector.x), (y / vector.y), (z / vector.z));
        }

        Float3 Float3::operator * (const Float3 &vector) const
        {
            return Float3((x * vector.x), (y * vector.y), (z * vector.z));
        }

        Float3 operator - (const Float3 &vector)
        {
            return Float3(-vector.x, -vector.y, -vector.z);
        }
    }; // namespace Math
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"

namespace Gek
{
    namespace Math
    {
        struct Quaternion
            : public Float4
        {
        public:
            static const Quaternion Identity;
            static const Quaternion Zero;

        public:
            inline static Quaternion MakeEulerRotation(float pitch, float yaw, float roll) noexcept
            {
                float sinPitch(std::sin(pitch * 0.5f));
                float sinYaw(std::sin(yaw * 0.5f));
                float sinRoll(std::sin(roll * 0.5f));
                float cosPitch(std::cos(pitch * 0.5f));
                float cosYaw(std::cos(yaw * 0.5f));
                float cosRoll(std::cos(roll * 0.5f));
                return Quaternion(
                    ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                    ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)));
            }

            inline static Quaternion MakeAngularRotation(Float3 const &axis, float radians) noexcept
            {
                float halfRadians = (radians * 0.5f);
                Float3 normal(axis.getNormal());
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    (normal.x * sinAngle),
                    (normal.y * sinAngle),
                    (normal.z * sinAngle),
                    std::cos(halfRadians));
            }

            inline static Quaternion MakePitchRotation(float radians) noexcept
            {
                float halfRadians = (radians * 0.5f);
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    sinAngle,
                    0.0f,
                    0.0f,
                    std::cos(halfRadians));
            }

            inline static Quaternion MakeYawRotation(float radians) noexcept
            {
                float halfRadians = (radians * 0.5f);
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    0.0f,
                    sinAngle,
                    0.0f,
                    std::cos(halfRadians));
            }

            inline static Quaternion MakeRollRotation(float radians) noexcept
            {
                float halfRadians = (radians * 0.5f);
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    0.0f,
                    0.0f,
                    sinAngle,
                    std::cos(halfRadians));
            }

        public:
            using Float4::Float4;
            using Float4::set;
            using Float4::dot;
            using Float4::getMagnitude;
            using Float4::getLength;
            using Float4::getNormal;
            using Float4::normalize;

            inline Quaternion(void) noexcept
            {
            }

            inline Quaternion(Quaternion const &quaternion) noexcept
                : Float4(quaternion)
            {
            }

            inline Float3 getEuler(void) const noexcept
            {
                float pitch;
                float yaw;
                float roll;
                float val = 2.0f * (y * w - z * x);
                if (val >= 0.99995f)
                {
                    pitch = 2.0f * std::atan2(x, w);
                    yaw = (0.5f * 3.141592f);
                    roll = 0.0f;
                }
                else if (val <= -0.99995f)
                {
                    pitch = 2.0f * std::atan2(x, w);
                    yaw = -(0.5f * 3.141592f);
                    roll = 0.0f;
                }
                else
                {
                    yaw = std::asin(val);
                    pitch = std::atan2(2.0f * (x * w + z * y), 1.0f - 2.0f * (x * x + y * y));
                    roll = std::atan2(2.0f * (z * w + x * y), 1.0f - 2.0f * (y * y + z * z));
                }

                return Float3(pitch, yaw, roll);
            }

            inline Quaternion getConjugate(void) const noexcept
            {
                return Quaternion(-x, -y, -z, w);
            }

            inline Quaternion getInverse(void) const noexcept
            {
                float inverseLength = (1.0f / getLength());
                return (*this * inverseLength);
            }

            inline void conjugate(void) noexcept
            {
                x *= -1.0f;
                y *= -1.0f;
                z *= -1.0f;
            }

            inline void invert(void) noexcept
            {
                float inverseLength = (1.0f / getLength());
                x *= -inverseLength;
                y *= -inverseLength;
                z *= -inverseLength;
            }

            inline Quaternion slerp(Quaternion const &quaternion, float factor) const noexcept
            {
                Quaternion result;
                float deltaAngle = dot(quaternion);
                if ((deltaAngle + 1.0f) > Epsilon)
                {
                    float factor0;
                    float factor1;
                    if (deltaAngle < 0.995f)
                    {
                        float acos = std::acos(deltaAngle);
                        float sin = std::sin(acos);
                        float denominator = 1.0f / sin;
                        factor0 = std::sin((1.0f - factor) * acos) * denominator;
                        factor1 = std::sin(factor * acos) * denominator;
                    }
                    else
                    {
                        factor0 = 1.0f - factor;
                        factor1 = factor;
                    }

                    result = ((*this) * factor0) + (quaternion * factor1);
                }
                else
                {
                    result.w = z;
                    result.x = -y;
                    result.y = x;
                    result.z = w;
                    float factor0 = std::sin((1.0f - factor) * (Pi * 0.5f));
                    float factor1 = std::sin(factor * (Pi * 0.5f));
                    result = ((*this) * factor0) + (quaternion * factor1);
                }

                deltaAngle = result.dot(result);
                if ((deltaAngle) < (1.0f - Epsilon))
                {
                    deltaAngle = 1.0f / std::sqrt(deltaAngle);
                    result *= deltaAngle;
                }

                return result;
            }

            inline bool operator == (Quaternion const &quaternion) const noexcept
            {
                if (x != quaternion.x) return false;
                if (y != quaternion.y) return false;
                if (z != quaternion.z) return false;
                if (w != quaternion.w) return false;
                return true;
            }

            inline bool operator != (Quaternion const &quaternion) const noexcept
            {
                if (x != quaternion.x) return true;
                if (y != quaternion.y) return true;
                if (z != quaternion.z) return true;
                if (w != quaternion.w) return true;
                return false;
            }

            inline operator const float *() const noexcept
            {
                return data;
            }

            inline Float3 rotate(Float3 const &vector) const noexcept
            {
                Float3 twoCross(2.0f * xyz().cross(vector));
                return (vector + (w * twoCross) + xyz().cross(twoCross));
            }

            inline Quaternion operator * (Quaternion const &quaternion) const noexcept
            {
                return Quaternion(
                    (w * quaternion.x + x * quaternion.w + y * quaternion.z - z * quaternion.y),
                    (w * quaternion.y - x * quaternion.z + y * quaternion.w + z * quaternion.x),
                    (w * quaternion.z + x * quaternion.y - y * quaternion.x + z * quaternion.w),
                    (w * quaternion.w - x * quaternion.x - y * quaternion.y - z * quaternion.z));
            }

            inline void operator *= (Quaternion const &quaternion) noexcept
            {
                (*this) = ((*this) * quaternion);
            }

            inline Quaternion &operator = (Quaternion const &quaternion) noexcept
            {
                x = quaternion.x;
                y = quaternion.y;
                z = quaternion.z;
                w = quaternion.w;
                return (*this);
            }

            inline void operator /= (float scalar) noexcept
            {
                float inverseScalar = (1.0f / scalar);
                x *= inverseScalar;
                y *= inverseScalar;
                z *= inverseScalar;
                w *= inverseScalar;
            }

            inline void operator *= (float scalar) noexcept
            {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                w *= scalar;
            }

            inline Quaternion operator / (float scalar) const noexcept
            {
                float inverseScalar = (1.0f / scalar);
                return Quaternion(
                    (x * inverseScalar),
                    (y * inverseScalar),
                    (z * inverseScalar),
                    (w * inverseScalar));
            }
            
            inline Quaternion operator + (float scalar) const noexcept
            {
                return Quaternion(
                    (x + scalar),
                    (y + scalar),
                    (z + scalar),
                    (w + scalar));
            }

            inline Quaternion operator * (float scalar) const noexcept
            {
                return Quaternion(
                    (x * scalar),
                    (y * scalar),
                    (z * scalar),
                    (w * scalar));
            }

            inline Quaternion operator + (Quaternion const &quaternion) const noexcept
            {
                return Quaternion(
                    (x + quaternion.x),
                    (y + quaternion.y),
                    (z + quaternion.z),
                    (w + quaternion.w));
            }
        };
    }; // namespace Math
}; // namespace Gek

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
        {
        public:
            static const Quaternion Identity;
            static const Quaternion Zero;

        public:
            union
            {
                struct { float data[4]; };
                struct
                {
                    union
                    {
                        struct { float x, y, z; };
                        Float3 axis;
                    };
                    
                    union
                    {
                        float w;
                        float angle;
                    };
                };
            };

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
            inline Quaternion(void) noexcept
            {
            }

            inline Quaternion(Quaternion const &rotation) noexcept
                : axis(rotation.axis)
                , angle(rotation.angle)
            {
            }

            explicit inline Quaternion(float x, float y, float z, float w) noexcept
                : x(x)
                , y(y)
                , z(z)
                , w(w)
            {
            }

            explicit inline Quaternion(float const * const data) noexcept
                : x(data[0])
                , y(data[1])
                , z(data[2])
                , w(data[3])
            {
            }

            explicit inline Quaternion(Math::Float3 const &axis, float angle) noexcept
                : axis(axis)
                , angle(angle)
            {
            }

            inline void set(float x, float y, float z, float w) noexcept
            {
                this->x = x;
                this->y = y;
                this->z = z;
                this->w = w;
            }

            inline void set(float const * constdata) noexcept
            {
                this->x = data[0];
                this->y = data[1];
                this->z = data[2];
                this->w = data[3];
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

            inline float getMagnitude(void) const noexcept
            {
                return dot(*this);
            }

            inline float getLength(void) const noexcept
            {
                return std::sqrt(getMagnitude());
            }

            inline Quaternion getNormal(void) const noexcept
            {
                float inverseLength = (1.0f / getLength());
                return ((*this) * inverseLength);
            }

            inline Quaternion getInverse(void) const noexcept
            {
                return Quaternion(-axis, angle);
            }

            inline void invert(void) noexcept
            {
                axis *= -1.0f;
            }

            inline void normalize(void) noexcept
            {
                float inverseLength = (1.0f / getLength());
                (*this) *= inverseLength;
            }

            inline float dot(Quaternion const &rotation) const noexcept
            {
                return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
            }

            inline Quaternion slerp(Quaternion const &rotation, float factor) const noexcept
            {
                Quaternion result;
                float deltaAngle = dot(rotation);
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

                    result = ((*this) * factor0) + (rotation * factor1);
                }
                else
                {
                    result.w = z;
                    result.x = -y;
                    result.y = x;
                    result.z = w;
                    float factor0 = std::sin((1.0f - factor) * (Pi * 0.5f));
                    float factor1 = std::sin(factor * (Pi * 0.5f));
                    result = ((*this) * factor0) + (rotation * factor1);
                }

                deltaAngle = result.dot(result);
                if ((deltaAngle) < (1.0f - Epsilon))
                {
                    deltaAngle = 1.0f / std::sqrt(deltaAngle);
                    result *= deltaAngle;
                }

                return result;
            }

            inline bool operator == (Quaternion const &rotation) const noexcept
            {
                if (x != rotation.x) return false;
                if (y != rotation.y) return false;
                if (z != rotation.z) return false;
                if (w != rotation.w) return false;
                return true;
            }

            inline bool operator != (Quaternion const &rotation) const noexcept
            {
                if (x != rotation.x) return true;
                if (y != rotation.y) return true;
                if (z != rotation.z) return true;
                if (w != rotation.w) return true;
                return false;
            }

            inline operator const float *() const noexcept
            {
                return data;
            }

            inline Float3 rotate(Float3 const &vector) const noexcept
            {
                Float3 twoCross(2.0f * axis.cross(vector));
                return (vector + (angle * twoCross) + axis.cross(twoCross));
            }

            inline Quaternion operator * (Quaternion const &rotation) const noexcept
            {
                return Quaternion(
                    (rotation.w * x) + (rotation.x * w) + (rotation.y * z) - (rotation.z * y),
                    (rotation.w * y) + (rotation.y * w) + (rotation.z * x) - (rotation.x * z),
                    (rotation.w * z) + (rotation.z * w) + (rotation.x * y) - (rotation.y * x),
                    (rotation.w * w) - (rotation.x * x) - (rotation.y * y) - (rotation.z * z));
            }

            inline void operator *= (Quaternion const &rotation) noexcept
            {
                (*this) = ((*this) * rotation);
            }

            inline Quaternion &operator = (Quaternion const &rotation) noexcept
            {
                axis = rotation.axis;
                angle = rotation.angle;
                return (*this);
            }

            inline void operator /= (float scalar) noexcept
            {
                axis /= scalar;
                angle /= scalar;
            }

            inline void operator *= (float scalar) noexcept
            {
                axis *= scalar;
                angle *= scalar;
            }

            inline Quaternion operator / (float scalar) const noexcept
            {
                return Quaternion((axis / scalar), (angle / scalar));
            }

            inline Quaternion operator + (float scalar) const noexcept
            {
                return Quaternion((axis + scalar), (angle + scalar));
            }

            inline Quaternion operator * (float scalar) const noexcept
            {
                return Quaternion((axis * scalar), (angle *scalar));
            }

            inline Quaternion operator + (Quaternion const &rotation) const noexcept
            {
                return Quaternion((axis + rotation.axis), (angle + rotation.angle));
            }
        };
    }; // namespace Math
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Common.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4.hpp"

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
                const struct { float x, y, z, w; };
                const struct { float data[4]; };
                const struct { Float3 axis; float angle; };
            };

        public:
            inline static Quaternion FromEuler(float pitch, float yaw, float roll)
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

            inline static Quaternion FromAngular(const Float3 &axis, float radians)
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

            inline static Quaternion FromPitch(float radians)
            {
                float halfRadians = (radians * 0.5f);
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    sinAngle,
                    0.0f,
                    0.0f,
                    std::cos(halfRadians));
            }

            inline static Quaternion FromYaw(float radians)
            {
                float halfRadians = (radians * 0.5f);
                float sinAngle(std::sin(halfRadians));
                return Quaternion(
                    0.0f,
                    sinAngle,
                    0.0f,
                    std::cos(halfRadians));
            }

            inline static Quaternion FromRoll(float radians)
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
            inline Quaternion(void)
            {
            }

            inline Quaternion(float x, float y, float z, float w)
                : x(x)
                , y(y)
                , z(z)
                , w(w)
            {
            }

            inline Quaternion(const float *data)
                : x(data[0])
                , y(data[1])
                , z(data[2])
                , w(data[3])
            {
            }

            inline Quaternion(const Quaternion &rotation)
                : axis(rotation.axis)
                , angle(rotation.angle)
            {
            }

            inline Quaternion(const Math::Float3 &axis, float angle)
                : axis(axis)
                , angle(angle)
            {
            }

            inline void set(float x, float y, float z, float w)
            {
                this->x = x;
                this->y = y;
                this->z = z;
                this->w = w;
            }

            inline void set(const float *data)
            {
                this->x = data[0];
                this->y = data[1];
                this->z = data[2];
                this->w = data[3];
            }

            inline Float3 getEuler(void) const
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

            inline float getMagnitudeSquared(void) const
            {
                return dot(*this);
            }

            inline float getMagnitude(void) const
            {
                return std::sqrt(getMagnitudeSquared());
            }

            inline Quaternion getNormal(void) const
            {
                float inverseMagnitude = (1.0f / getMagnitude());
                return ((*this) * inverseMagnitude);
            }

            inline Quaternion getInverse(void) const
            {
                return Quaternion(-axis, angle);
            }

            inline void invert(void)
            {
                axis *= -1.0f;
            }

            inline void normalize(void)
            {
                float inverseMagnitude = (1.0f / getMagnitude());
                (*this) *= inverseMagnitude;
            }

            inline float dot(const Quaternion &rotation) const
            {
                return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
            }

            inline Quaternion slerp(const Quaternion &rotation, float factor) const
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

            inline bool operator == (const Quaternion &rotation) const
            {
                if (x != rotation.x) return false;
                if (y != rotation.y) return false;
                if (z != rotation.z) return false;
                if (w != rotation.w) return false;
                return true;
            }

            inline bool operator != (const Quaternion &rotation) const
            {
                if (x != rotation.x) return true;
                if (y != rotation.y) return true;
                if (z != rotation.z) return true;
                if (w != rotation.w) return true;
                return false;
            }

            inline operator const float *() const
            {
                return data;
            }

            inline Float3 rotate(const Float3 &vector) const
            {
                Float3 twoCross(2.0f * axis.cross(vector));
                return (vector + (angle * twoCross) + axis.cross(twoCross));
            }

            inline Quaternion operator * (const Quaternion &rotation) const
            {
                return Quaternion(
                    rotation.x * w + rotation.w * x - rotation.z * y + rotation.y * z,
                    rotation.y * w + rotation.z * x + rotation.w * y - rotation.x * z,
                    rotation.z * w - rotation.y * x + rotation.x * y + rotation.w * z,
                    rotation.w * w - rotation.x * x - rotation.y * y - rotation.z * z);
            }

            inline void operator *= (const Quaternion &rotation)
            {
                (*this) = ((*this) * rotation);
            }

            inline Quaternion &operator = (const Quaternion &rotation)
            {
                axis = rotation.axis;
                angle = rotation.angle;
                return (*this);
            }

            inline void operator /= (float scalar)
            {
                axis /= scalar;
                angle /= scalar;
            }

            inline void operator *= (float scalar)
            {
                axis *= scalar;
                angle *= scalar;
            }

            inline Quaternion operator / (float scalar) const
            {
                return Quaternion((axis / scalar), (angle / scalar));
            }

            inline Quaternion operator + (float scalar) const
            {
                return Quaternion((axis + scalar), (angle + scalar));
            }

            inline Quaternion operator * (float scalar) const
            {
                return Quaternion((axis * scalar), (angle *scalar));
            }

            inline Quaternion operator + (const Quaternion &rotation) const
            {
                return Quaternion((axis + rotation.axis), (angle + rotation.angle));
            }
        };
    }; // namespace Math
}; // namespace Gek

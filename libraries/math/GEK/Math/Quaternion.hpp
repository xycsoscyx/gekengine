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
                float angle = dot(rotation);
                if ((angle + 1.0f) > 1.0e-5)
                {
                    float Sclp;
                    float Sclq;
                    if (angle < 0.995f)
                    {
                        float ang = std::acos(angle);
                        float sinAng = std::sin(ang);

                        float den = 1.0f / sinAng;

                        Sclp = std::sin((1.0f - factor) * ang) * den;
                        Sclq = std::sin(factor * ang) * den;

                    }
                    else
                    {
                        Sclp = 1.0f - factor;
                        Sclq = factor;
                    }

                    result.w = w * Sclp + rotation.w * Sclq;
                    result.x = x * Sclp + rotation.x * Sclq;
                    result.y = y * Sclp + rotation.y * Sclq;
                    result.z = z * Sclp + rotation.z * Sclq;

                }
                else
                {
                    result.w = z;
                    result.x = -y;
                    result.y = x;
                    result.z = w;

                    float Sclp = std::sin((1.0f - factor) * (3.141592f *0.5f));
                    float Sclq = std::sin(factor * (3.141592f * 0.5f));

                    result.w = w * Sclp + result.w * Sclq;
                    result.x = x * Sclp + result.x * Sclq;
                    result.y = y * Sclp + result.y * Sclq;
                    result.z = z * Sclp + result.z * Sclq;
                }

                angle = result.dot(result);
                if ((angle) < (1.0f - 1.0e-4f))
                {
                    angle = 1.0f / std::sqrt(angle);
                    result.w *= angle;
                    result.x *= angle;
                    result.y *= angle;
                    result.z *= angle;
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
                    ((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                    ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                    ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                    ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z))).getNormal();
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

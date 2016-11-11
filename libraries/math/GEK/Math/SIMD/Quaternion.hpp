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
#include <xmmintrin.h>
#include <type_traits>

namespace Gek
{
    namespace Math
    {
        namespace SIMD
        {
            struct Quaternion
            {
            public:
                static const Quaternion Identity;

            public:
                union
                {
                    struct { float x, y, z, w; };
                    struct { float data[4]; };
                    struct { __m128 simd; };
                    struct { Float3 vector; float w; };
                };

            public:
                inline static Quaternion createEulerRotation(float pitch, float yaw, float roll)
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

                inline static Quaternion createAngularRotation(const Float3 &axis, float radians)
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

                inline static Quaternion createPitchRotation(float radians)
                {
                    float halfRadians = (radians * 0.5f);
                    float sinAngle(std::sin(halfRadians));
                    return Quaternion(
                        sinAngle,
                        0.0f,
                        0.0f,
                        std::cos(halfRadians));
                }

                inline static Quaternion createYawRotation(float radians)
                {
                    float halfRadians = (radians * 0.5f);
                    float sinAngle(std::sin(halfRadians));
                    return Quaternion(
                        0.0f,
                        sinAngle,
                        0.0f,
                        std::cos(halfRadians));
                }

                inline static Quaternion createRollRotation(float radians)
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

                inline Quaternion(__m128 simd)
                    : simd(simd)
                {
                }

                inline Quaternion(float x, float y, float z, float w)
                    : simd(_mm_setr_ps(x, y, z, w))
                {
                }

                inline Quaternion(const float *data)
                    : simd(_mm_loadu_ps(data))
                {
                }

                inline Quaternion(const Quaternion &vector)
                    : simd(vector.simd)
                {
                }

                inline Quaternion &set(float x, float y, float z, float w)
                {
                    simd = _mm_setr_ps(x, y, z, w);
                    return (*this);
                }

                inline float getLengthSquared(void) const
                {
                    return dot(*this);
                }

                inline float getLength(void) const
                {
                    return std::sqrt(getLengthSquared());
                }

                inline Quaternion getNormal(void) const
                {
                    auto length = _mm_set1_ps(getLength());
                    return _mm_mul_ps(simd, _mm_rcp_ps(length));
                }

                inline Quaternion getInverse(void) const
                {
                    return Quaternion(-x, -y, -z, w);
                }

                inline Quaternion integrateOmega(const Float3 &omega, float deltaTime) const
                {
                    // this is correct
                    Quaternion rotation(*this);
                    float omegaMagnitudeSquared = omega.dot(omega);
                    const float errorAngle = convertDegreesToRadians(float(0.0125));
                    const float errorAngleSquared = (errorAngle * errorAngle);
                    if (omegaMagnitudeSquared > errorAngleSquared)
                    {
                        float inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                        Float3 omegaAxis(omega * inverseOmegaMagnitude);
                        float omegaAngle = inverseOmegaMagnitude * omegaMagnitudeSquared * deltaTime;
                        Quaternion deltaRotation(createAngularRotation(omegaAxis, omegaAngle));
                        rotation = rotation * deltaRotation;
                        rotation *= (float(1) / std::sqrt(rotation.dot(rotation)));
                    }

                    return rotation;
                }

                inline Float3 calculateAverageOmega(const Quaternion &rotationTarget, float inverseDeltaTime) const
                {
                    Quaternion rotationSource(*this);
                    if (rotationSource.dot(rotationTarget) < 0.0f)
                    {
                        rotationSource *= -1.0f;
                    }

                    Quaternion deltaRotation(rotationSource.getInverse() * rotationTarget);
                    Float3 omegaDirection(deltaRotation.vector);

                    float omegaMagnitudeSquared = omegaDirection.dot(omegaDirection);
                    if (omegaMagnitudeSquared	< float(float(1.0e-5f) * float(1.0e-5f)))
                    {
                        return Float3::Zero;
                    }

                    float inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                    float directionMagnitude = omegaMagnitudeSquared * inverseOmegaMagnitude;

                    float omegaMagnitude = float(2.0f) * std::atan2(directionMagnitude, deltaRotation.w) * inverseDeltaTime;
                    return omegaDirection * (inverseOmegaMagnitude * omegaMagnitude);
                }


                inline void invert(void)
                {
                    x = -x;
                    y = -y;
                    z = -z;
                }

                inline void normalize(void)
                {
                    (*this) = getNormal();
                }

                inline float dot(const Quaternion &rotation) const
                {
                    return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
                }

                inline Quaternion slerp(const Quaternion &rotation, float factor) const
                {
                    float omega = std::acos(clamp(dot(rotation), -1.0f, 1.0f));
                    if (std::abs(omega) < 1e-10f)
                    {
                        omega = 1e-10f;
                    }

                    float sinTheta = std::sin(omega);
                    float factor0 = (std::sin((1.0f - factor) * omega) / sinTheta);
                    float factor1 = (std::sin(factor * omega) / sinTheta);
                    return blend((*this), factor0, rotation, factor1);
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

                inline operator float *()
                {
                    return data;
                }

                inline Float3 operator * (const Float3 &vector) const
                {
                    Float3 cross(2.0f * this->vector.cross(vector));
                    return (vector + (this->w * cross) + this->vector.cross(cross));
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
                    simd = rotation.simd;
                    return (*this);
                }

                inline void operator /= (float scalar)
                {
                    simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
                }

                inline void operator *= (float scalar)
                {
                    simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
                }

                inline Quaternion operator / (float scalar) const
                {
                    return _mm_div_ps(simd, _mm_set1_ps(scalar));
                }

                inline Quaternion operator + (float scalar) const
                {
                    return _mm_add_ps(simd, _mm_set1_ps(scalar));
                }

                inline Quaternion operator * (float scalar) const
                {
                    return _mm_mul_ps(simd, _mm_set1_ps(scalar));
                }

                inline Quaternion operator + (const Quaternion &rotation) const
                {
                    return _mm_add_ps(simd, rotation.simd);
                }
            };
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

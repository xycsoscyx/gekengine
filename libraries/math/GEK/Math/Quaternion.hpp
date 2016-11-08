/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Vector3.hpp"
#include <xmmintrin.h>
#include <type_traits>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        struct BaseQuaternion
        {
        public:
            static const BaseQuaternion Identity;

        public:
            union
            {
                struct { TYPE x, y, z, w; };
                struct { TYPE data[4]; };
                struct { __m128 simd; };
                struct { Vector3<TYPE> vector; TYPE w; };
            };

        public:
            BaseQuaternion(void)
            {
            }

            BaseQuaternion(__m128 simd)
                : simd(simd)
            {
            }

            BaseQuaternion(TYPE x, TYPE y, TYPE z, TYPE w)
                : simd(_mm_setr_ps(x, y, z, w))
            {
            }

            BaseQuaternion(const TYPE *data)
                : simd(_mm_loadu_ps(data))
            {
            }

            BaseQuaternion(const BaseQuaternion &vector)
                : simd(vector.simd)
            {
            }

            BaseQuaternion &set(TYPE x, TYPE y, TYPE z, TYPE w)
            {
                simd = _mm_setr_ps(x, y, z, w);
                return (*this);
            }

            static BaseQuaternion createEulerRotation(TYPE pitch, TYPE yaw, TYPE roll)
            {
                TYPE sinPitch(std::sin(pitch * 0.5f));
                TYPE sinYaw(std::sin(yaw * 0.5f));
                TYPE sinRoll(std::sin(roll * 0.5f));
                TYPE cosPitch(std::cos(pitch * 0.5f));
                TYPE cosYaw(std::cos(yaw * 0.5f));
                TYPE cosRoll(std::cos(roll * 0.5f));
                return BaseQuaternion(
                    ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                    ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)));
            }

            static BaseQuaternion createAngularRotation(const Vector3<TYPE> &axis, TYPE radians)
            {
                TYPE halfRadians = (radians * 0.5f);
                Vector3<TYPE> normal(axis.getNormal());
                TYPE sinAngle(std::sin(halfRadians));
                return BaseQuaternion(
                    (normal.x * sinAngle),
                    (normal.y * sinAngle),
                    (normal.z * sinAngle),
                    std::cos(halfRadians));
            }

            TYPE getLengthSquared(void) const
            {
                return dot(*this);
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            BaseQuaternion getNormal(void) const
            {
                auto length = _mm_set1_ps(getLength());
                return _mm_mul_ps(simd, _mm_rcp_ps(length));
            }

            BaseQuaternion getInverse(void) const
            {
                return BaseQuaternion(-x, -y, -z, w);
            }

            BaseQuaternion integrateOmega(const Vector3<TYPE> &omega, TYPE deltaTime) const
            {
                // this is correct
                BaseQuaternion rotation(*this);
                TYPE omegaMagnitudeSquared = omega.dot(omega);
                const TYPE errorAngle = convertDegreesToRadians(TYPE(0.0125));
                const TYPE errorAngleSquared = (errorAngle * errorAngle);
                if (omegaMagnitudeSquared > errorAngleSquared)
                {
                    TYPE inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                    Vector3<TYPE> omegaAxis(omega * inverseOmegaMagnitude);
                    TYPE omegaAngle = inverseOmegaMagnitude * omegaMagnitudeSquared * deltaTime;
                    BaseQuaternion deltaRotation(createAngularRotation(omegaAxis, omegaAngle));
                    rotation = rotation * deltaRotation;
                    rotation *= (TYPE(1) / std::sqrt(rotation.dot(rotation)));
                }

                return rotation;
            }

            Vector3<TYPE> calculateAverageOmega(const BaseQuaternion &rotationTarget, TYPE inverseDeltaTime) const
            {
                BaseQuaternion rotationSource(*this);
                if (rotationSource.dot(rotationTarget) < 0.0f)
                {
                    rotationSource *= -1.0f;
                }

                BaseQuaternion deltaRotation(rotationSource.getInverse() * rotationTarget);
                Vector3<TYPE> omegaDirection(deltaRotation.vector);

                TYPE omegaMagnitudeSquared = omegaDirection.dot(omegaDirection);
                if (omegaMagnitudeSquared	< TYPE(TYPE(1.0e-5f) * TYPE(1.0e-5f)))
                {
                    return Vector3<TYPE>::Zero;
                }

                TYPE inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                TYPE directionMagnitude = omegaMagnitudeSquared * inverseOmegaMagnitude;

                TYPE omegaMagnitude = TYPE(2.0f) * std::atan2(directionMagnitude, deltaRotation.w) * inverseDeltaTime;
                return omegaDirection * (inverseOmegaMagnitude * omegaMagnitude);
            }


            void invert(void)
            {
                x = -x;
                y = -y;
                z = -z;
            }

            void normalize(void)
            {
                (*this) = getNormal();
            }

            TYPE dot(const BaseQuaternion &rotation) const
            {
                return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
            }

            BaseQuaternion slerp(const BaseQuaternion &rotation, TYPE factor) const
            {
                TYPE omega = std::acos(clamp(dot(rotation), -1.0f, 1.0f));
                if (std::abs(omega) < 1e-10f)
                {
                    omega = 1e-10f;
                }

                TYPE sinTheta = std::sin(omega);
                TYPE factor0 = (std::sin((1.0f - factor) * omega) / sinTheta);
                TYPE factor1 = (std::sin(factor * omega) / sinTheta);
                return blend((*this), factor0, rotation, factor1);
            }

            bool operator == (const BaseQuaternion &rotation) const
            {
                if (x != rotation.x) return false;
                if (y != rotation.y) return false;
                if (z != rotation.z) return false;
                if (w != rotation.w) return false;
                return true;
            }

            bool operator != (const BaseQuaternion &rotation) const
            {
                if (x != rotation.x) return true;
                if (y != rotation.y) return true;
                if (z != rotation.z) return true;
                if (w != rotation.w) return true;
                return false;
            }

            operator const TYPE *() const
            {
                return data;
            }

            operator TYPE *()
            {
                return data;
            }

            Vector3<TYPE> operator * (const Vector3<TYPE> &vector) const
            {
                Vector3<TYPE> cross(2.0f * this->vector.cross(vector));
                return (vector + (this->w * cross) + this->vector.cross(cross));
            }

            BaseQuaternion operator * (const BaseQuaternion &rotation) const
            {
                return BaseQuaternion(
                    ((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                    ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                    ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                    ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z))).getNormal();
            }

            void operator *= (const BaseQuaternion &rotation)
            {
                (*this) = ((*this) * rotation);
            }

            BaseQuaternion &operator = (const BaseQuaternion &rotation)
            {
                simd = rotation.simd;
                return (*this);
            }

            void operator /= (TYPE scalar)
            {
                simd = _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            void operator *= (TYPE scalar)
            {
                simd = _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            BaseQuaternion operator / (TYPE scalar) const
            {
                return _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            BaseQuaternion operator + (TYPE scalar) const
            {
                return _mm_add_ps(simd, _mm_set1_ps(scalar));
            }

            BaseQuaternion operator * (TYPE scalar) const
            {
                return _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            BaseQuaternion operator + (const BaseQuaternion &rotation) const
            {
                return _mm_add_ps(simd, rotation.simd);
            }
        };

        using Quaternion = BaseQuaternion<float>;
    }; // namespace Math
}; // namespace Gek

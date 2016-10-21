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
        struct Quaternion
        {
        public:
            static const Quaternion Identity;

        public:
            union
            {
                struct { TYPE x, y, z, w; };
                struct { TYPE data[4]; };
                struct { __m128 simd; };
                struct { Vector3<TYPE> vector; TYPE w; };
            };

        public:
            Quaternion(void)
            {
            }

            Quaternion(__m128 simd)
                : simd(simd)
            {
            }

            Quaternion(const TYPE(&data)[4])
                : simd(_mm_loadu_ps(data))
            {
            }

            Quaternion(const TYPE *data)
                : simd(_mm_loadu_ps(data))
            {
            }

            Quaternion(const Quaternion &vector)
                : simd(vector.simd)
            {
            }

            Quaternion(TYPE x, TYPE y, TYPE z, TYPE w)
                : data{ x, y, z, w }
            {
            }

            Quaternion &set(TYPE x, TYPE y, TYPE z, TYPE w)
            {
                simd = _mm_setr_ps(x, y, z, w);
                return (*this);
            }

            static Quaternion createEulerRotation(TYPE pitch, TYPE yaw, TYPE roll)
            {
                TYPE sinPitch(std::sin(pitch * 0.5f));
                TYPE sinYaw(std::sin(yaw * 0.5f));
                TYPE sinRoll(std::sin(roll * 0.5f));
                TYPE cosPitch(std::cos(pitch * 0.5f));
                TYPE cosYaw(std::cos(yaw * 0.5f));
                TYPE cosRoll(std::cos(roll * 0.5f));
                return Quaternion(
                {
                    ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                    ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                    ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)),
                });
            }

            static Quaternion createAngularRotation(const Vector3<TYPE> &axis, TYPE radians)
            {
                TYPE halfRadians = (radians * 0.5f);
                Vector3<TYPE> normal(axis.getNormal());
                TYPE sinAngle(std::sin(halfRadians));
                return Quaternion(
                {
                    (normal.x * sinAngle),
                    (normal.y * sinAngle),
                    (normal.z * sinAngle),
                    std::cos(halfRadians),
                });
            }

            TYPE getLengthSquared(void) const
            {
                return dot(*this);
            }

            TYPE getLength(void) const
            {
                return std::sqrt(getLengthSquared());
            }

            Quaternion getNormal(void) const
            {
                return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
            }

            Quaternion getInverse(void) const
            {
                return Quaternion(-x, -y, -z, w);
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

            TYPE dot(const Quaternion &rotation) const
            {
                return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
            }

            Quaternion slerp(const Quaternion &rotation, TYPE factor) const
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

            bool operator == (const Quaternion &rotation) const
            {
                if (x != rotation.x) return false;
                if (y != rotation.y) return false;
                if (z != rotation.z) return false;
                if (w != rotation.w) return false;
                return true;
            }

            bool operator != (const Quaternion &rotation) const
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

            Quaternion operator * (const Quaternion &rotation) const
            {
                return Quaternion(((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                    ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                    ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                    ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z))).getNormal();
            }

            void operator *= (const Quaternion &rotation)
            {
                (*this) = ((*this) * rotation);
            }

            Quaternion &operator = (const Quaternion &rotation)
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

            Quaternion operator / (TYPE scalar) const
            {
                return _mm_div_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator + (TYPE scalar) const
            {
                return _mm_add_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator * (TYPE scalar) const
            {
                return _mm_mul_ps(simd, _mm_set1_ps(scalar));
            }

            Quaternion operator + (const Quaternion &rotation) const
            {
                return _mm_add_ps(simd, rotation.simd);
            }
        };

        using QuaternionFloat = Quaternion<float>;
    }; // namespace Math
}; // namespace Gek

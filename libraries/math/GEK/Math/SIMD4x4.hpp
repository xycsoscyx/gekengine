/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK\Math\Constants.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\SIMD4.hpp"
#include <xmmintrin.h>
#include <type_traits>

namespace Gek
{
    namespace Math
    {
        struct SIMD4x4
        {
        public:
            static const SIMD4x4 Identity;

        public:
            union
            {
                struct { float data[16]; };
                struct { float table[4][4]; };
                struct { SIMD4 rows[4]; };
                struct { __m128 simd[4]; };

                struct
                {
                    float _11, _12, _13, _14;
                    float _21, _22, _23, _24;
                    float _31, _32, _33, _34;
                    float _41, _42, _43, _44;
                };

                struct
                {
                    SIMD4 rx;
                    SIMD4 ry;
                    SIMD4 rz;
                    union
                    {
                        SIMD4 rw;
                        SIMD4 translation;
                    };
                };
            };

        public:
            static SIMD4x4 createScaling(const Float3 &scale, const Float3 &translation = Math::Float3::Zero)
            {
                return SIMD4x4(
                    scale.x, 0.0f, 0.0f, 0.0f,
                    0.0f, scale.y, 0.0f, 0.0f,
                    0.0f, 0.0f, scale.z, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            static SIMD4x4 createAngularRotation(const Float3 &axis, float radians, const Float3 &translation = Math::Float3::Zero)
            {
                // do the trig
                float cosAngle = cos(radians);
                float sinAngle = sin(radians);
                float theta = (1.0f - cosAngle);

                // build the rotation matrix
                return SIMD4x4(
                    theta*axis.x*axis.x + cosAngle, theta*axis.x*axis.y + sinAngle*axis.z, theta*axis.x*axis.z - sinAngle*axis.y, 0.0f,
                    theta*axis.x*axis.y - sinAngle*axis.z, theta*axis.y*axis.y + cosAngle, theta*axis.y*axis.z + sinAngle*axis.x, 0.0f,
                    theta*axis.x*axis.z + sinAngle*axis.y, theta*axis.y*axis.z - sinAngle*axis.x, theta*axis.z*axis.z + cosAngle, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            static SIMD4x4 createEulerRotation(float pitch, float yaw, float roll, const Float3 &translation = Math::Float3::Zero)
            {
                float cosPitch(std::cos(pitch));
                float sinPitch(std::sin(pitch));
                float cosYaw(std::cos(yaw));
                float sinYaw(std::sin(yaw));
                float cosRoll(std::cos(roll));
                float sinRoll(std::sin(roll));

                return SIMD4x4(
                    cosYaw * cosRoll, cosYaw * sinRoll, -sinYaw, 0.0f,
                    cosRoll * sinPitch * sinYaw - cosPitch * sinRoll, cosPitch * cosRoll + sinPitch * sinYaw * sinRoll, cosYaw * sinPitch, 0.0f,
                    cosPitch * cosRoll * sinYaw + sinPitch * sinRoll, cosPitch * sinYaw * sinRoll - cosRoll * sinPitch, cosPitch * cosYaw, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            static SIMD4x4 createPitchRotation(float radians, const Float3 &translation = Math::Float3::Zero)
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return SIMD4x4(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cosAngle, sinAngle, 0.0f,
                    0.0f, -sinAngle, cosAngle, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            static SIMD4x4 createYawRotation(float radians, const Float3 &translation = Math::Float3::Zero)
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return SIMD4x4(
                    cosAngle, 0.0f, -sinAngle, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    sinAngle, 0.0f, cosAngle, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            static SIMD4x4 createRollRotation(float radians, const Float3 &translation = Math::Float3::Zero)
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return SIMD4x4(
                    cosAngle, sinAngle, 0.0f, 0.0f,
                    -sinAngle, cosAngle, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
            static SIMD4x4 createOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip)
            {
                return SIMD4x4(
                    (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                    0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                    0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f,
                    ((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f);
            }

            // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
            static SIMD4x4 createPerspective(float fieldOfView, float aspectRatio, float nearClip, float farClip)
            {
                float yScale(1.0f / std::tan(fieldOfView * 0.5f));
                float xScale(yScale / aspectRatio);
                float denominator(farClip - nearClip);

                return SIMD4x4(
                    xScale, 0.0f, 0.0f, 0.0f,
                    0.0f, yScale, 0.0f, 0.0f,
                    0.0f, 0.0f, (farClip / denominator), 1.0f,
                    0.0f, 0.0f, ((-nearClip * farClip) / denominator), 0.0f);
            }

            static SIMD4x4 createLookAt(const Float3 &source, const Float3 &target, const Float3 &worldUpVector, const Float3 &translation = Math::Float3::Zero)
            {
                Float3 forward((target - source).getNormal());
                Float3 left(worldUpVector.cross(forward).getNormal());
                Float3 up(forward.cross(left));
                return SIMD4x4(
                    left.x, left.y, left.z, 0.0f,
                    up.x, up.y, up.z, 0.0f,
                    forward.x, forward.y, forward.z, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

        public:
            SIMD4x4(void)
            {
            }

            SIMD4x4(
                float _11, float _12, float _13, float _14,
                float _21, float _22, float _23, float _24,
                float _31, float _32, float _33, float _34,
                float _41, float _42, float _43, float _44)
                : simd{
                    _mm_setr_ps(_11, _12, _13, _14),
                    _mm_setr_ps(_21, _22, _23, _24),
                    _mm_setr_ps(_31, _32, _33, _34),
                    _mm_setr_ps(_41, _42, _43, _44) }
            {
            }

            SIMD4x4(const float *data)
                : simd{
                    _mm_loadu_ps(data + 0),
                    _mm_loadu_ps(data + 4),
                    _mm_loadu_ps(data + 8),
                    _mm_loadu_ps(data + 12) }
            {
            }

            SIMD4x4(
                const __m128 &rx, 
                const __m128 &ry,
                const __m128 &rz,
                const __m128 &rw)
                : simd{ rx, ry, rz, rw }
            {
            }

            SIMD4x4(const SIMD4x4 &matrix)
                : simd{ matrix.simd[0], matrix.simd[1], matrix.simd[2], matrix.simd[3] }
            {
            }

            Float3 getScaling(void) const
            {
                return Float3(_11, _22, _33);
            }

            float getDeterminant(void) const
            {
                return (
                    (table[0][0] * table[1][1] - table[1][0] * table[0][1]) *
                    (table[2][2] * table[3][3] - table[3][2] * table[2][3]) -
                    (table[0][0] * table[2][1] - table[2][0] * table[0][1]) *
                    (table[1][2] * table[3][3] - table[3][2] * table[1][3]) +
                    (table[0][0] * table[3][1] - table[3][0] * table[0][1]) *
                    (table[1][2] * table[2][3] - table[2][2] * table[1][3]) +
                    (table[1][0] * table[2][1] - table[2][0] * table[1][1]) *
                    (table[0][2] * table[3][3] - table[3][2] * table[0][3]) -
                    (table[1][0] * table[3][1] - table[3][0] * table[1][1]) *
                    (table[0][2] * table[2][3] - table[2][2] * table[0][3]) +
                    (table[2][0] * table[3][1] - table[3][0] * table[2][1]) *
                    (table[0][2] * table[1][3] - table[1][2] * table[0][3]));
            }

            SIMD4x4 getTranspose(void) const
            {
                __m128 tmp3, tmp2, tmp1, tmp0;
                tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
                tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
                tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
                tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
                return SIMD4x4(
                    _mm_shuffle_ps(tmp0, tmp1, 0x88),
                    _mm_shuffle_ps(tmp0, tmp1, 0xDD),
                    _mm_shuffle_ps(tmp2, tmp3, 0x88),
                    _mm_shuffle_ps(tmp2, tmp3, 0xDD));
            }

            SIMD4x4 getInverse(void) const
            {
                float determinant(getDeterminant());
                if (std::abs(determinant) < Epsilon)
                {
                    return Identity;
                }
                else
                {
                    determinant = (1.0f / determinant);

                    return SIMD4x4(
                        (determinant * (table[1][1] * (table[2][2] * table[3][3] - table[3][2] * table[2][3]) + table[2][1] * (table[3][2] * table[1][3] - table[1][2] * table[3][3]) + table[3][1] * (table[1][2] * table[2][3] - table[2][2] * table[1][3]))),
                        (determinant * (table[1][2] * (table[2][0] * table[3][3] - table[3][0] * table[2][3]) + table[2][2] * (table[3][0] * table[1][3] - table[1][0] * table[3][3]) + table[3][2] * (table[1][0] * table[2][3] - table[2][0] * table[1][3]))),
                        (determinant * (table[1][3] * (table[2][0] * table[3][1] - table[3][0] * table[2][1]) + table[2][3] * (table[3][0] * table[1][1] - table[1][0] * table[3][1]) + table[3][3] * (table[1][0] * table[2][1] - table[2][0] * table[1][1]))),
                        (determinant * (table[1][0] * (table[3][1] * table[2][2] - table[2][1] * table[3][2]) + table[2][0] * (table[1][1] * table[3][2] - table[3][1] * table[1][2]) + table[3][0] * (table[2][1] * table[1][2] - table[1][1] * table[2][2]))),
                        (determinant * (table[2][1] * (table[0][2] * table[3][3] - table[3][2] * table[0][3]) + table[3][1] * (table[2][2] * table[0][3] - table[0][2] * table[2][3]) + table[0][1] * (table[3][2] * table[2][3] - table[2][2] * table[3][3]))),
                        (determinant * (table[2][2] * (table[0][0] * table[3][3] - table[3][0] * table[0][3]) + table[3][2] * (table[2][0] * table[0][3] - table[0][0] * table[2][3]) + table[0][2] * (table[3][0] * table[2][3] - table[2][0] * table[3][3]))),
                        (determinant * (table[2][3] * (table[0][0] * table[3][1] - table[3][0] * table[0][1]) + table[3][3] * (table[2][0] * table[0][1] - table[0][0] * table[2][1]) + table[0][3] * (table[3][0] * table[2][1] - table[2][0] * table[3][1]))),
                        (determinant * (table[2][0] * (table[3][1] * table[0][2] - table[0][1] * table[3][2]) + table[3][0] * (table[0][1] * table[2][2] - table[2][1] * table[0][2]) + table[0][0] * (table[2][1] * table[3][2] - table[3][1] * table[2][2]))),
                        (determinant * (table[3][1] * (table[0][2] * table[1][3] - table[1][2] * table[0][3]) + table[0][1] * (table[1][2] * table[3][3] - table[3][2] * table[1][3]) + table[1][1] * (table[3][2] * table[0][3] - table[0][2] * table[3][3]))),
                        (determinant * (table[3][2] * (table[0][0] * table[1][3] - table[1][0] * table[0][3]) + table[0][2] * (table[1][0] * table[3][3] - table[3][0] * table[1][3]) + table[1][2] * (table[3][0] * table[0][3] - table[0][0] * table[3][3]))),
                        (determinant * (table[3][3] * (table[0][0] * table[1][1] - table[1][0] * table[0][1]) + table[0][3] * (table[1][0] * table[3][1] - table[3][0] * table[1][1]) + table[1][3] * (table[3][0] * table[0][1] - table[0][0] * table[3][1]))),
                        (determinant * (table[3][0] * (table[1][1] * table[0][2] - table[0][1] * table[1][2]) + table[0][0] * (table[3][1] * table[1][2] - table[1][1] * table[3][2]) + table[1][0] * (table[0][1] * table[3][2] - table[3][1] * table[0][2]))),
                        (determinant * (table[0][1] * (table[2][2] * table[1][3] - table[1][2] * table[2][3]) + table[1][1] * (table[0][2] * table[2][3] - table[2][2] * table[0][3]) + table[2][1] * (table[1][2] * table[0][3] - table[0][2] * table[1][3]))),
                        (determinant * (table[0][2] * (table[2][0] * table[1][3] - table[1][0] * table[2][3]) + table[1][2] * (table[0][0] * table[2][3] - table[2][0] * table[0][3]) + table[2][2] * (table[1][0] * table[0][3] - table[0][0] * table[1][3]))),
                        (determinant * (table[0][3] * (table[2][0] * table[1][1] - table[1][0] * table[2][1]) + table[1][3] * (table[0][0] * table[2][1] - table[2][0] * table[0][1]) + table[2][3] * (table[1][0] * table[0][1] - table[0][0] * table[1][1]))),
                        (determinant * (table[0][0] * (table[1][1] * table[2][2] - table[2][1] * table[1][2]) + table[1][0] * (table[2][1] * table[0][2] - table[0][1] * table[2][2]) + table[2][0] * (table[0][1] * table[1][2] - table[1][1] * table[0][2]))));
                }
            }

            SIMD4x4 getRotation(void) const
            {
                // Sets row/column 4 to identity
                return SIMD4x4(
                    _11, _12, _13, 0.0f,
                    _21, _22, _32, 0.0f,
                    _31, _32, _33, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
            }

            SIMD4x4 &transpose(void)
            {
                __m128 tmp3, tmp2, tmp1, tmp0;
                tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
                tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
                tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
                tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
                simd[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
                simd[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
                simd[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
                simd[3] = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
                return (*this);
            }

            SIMD4x4 &invert(void)
            {
                (*this) = getInverse();
                return (*this);
            }

            SIMD4 operator [] (int row) const
            {
                return rows[row];
            }

            SIMD4 &operator [] (int row)
            {
                return rows[row];
            }

            operator const float *() const
            {
                return data;
            }

            operator float *()
            {
                return data;
            }

            SIMD4x4 &operator = (const SIMD4x4 &matrix)
            {
                rows[0] = matrix.rows[0];
                rows[1] = matrix.rows[1];
                rows[2] = matrix.rows[2];
                rows[3] = matrix.rows[3];
                return (*this);
            }

            void operator *= (const SIMD4x4 &matrix)
            {
                simd[0] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                    _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                    _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                simd[1] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                    _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                    _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                simd[2] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                    _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                    _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                simd[3] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                    _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                    _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
            }

            SIMD4x4 operator * (const SIMD4x4 &matrix) const
            {
                return SIMD4x4(
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))));
            }

            Float3 rotate(const Float3 &vector) const
            {
                return Float3(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)));
            }

            Float3 transform(const Float3 &vector) const
            {
                return Float3(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + _41),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + _42),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + _43));
            }

            SIMD4 transform(const SIMD4 &vector) const
            {
                return SIMD4(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                    ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)));
            }
        };
    }; // namespace Math
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/SIMD/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include <xmmintrin.h>

namespace Gek
{
    namespace Math
    {
        namespace SIMD
        {
            __declspec(align(16))
            struct Float4x4
            {
            public:
                static const Float4x4 Identity;

            public:
                union
                {
                    struct { __m128 simd[4]; };
                    const struct { float data[16]; };
                    const struct { float table[4][4]; };
                    const struct { Float4 rows[4]; };
                    const struct { Float4 rx, ry, rz, rw; };
                    const struct
                    {
                        Float4 rx;
                        Float4 ry;
                        Float4 rz;
                        union
                        {
                            struct { Float4 rw; };
                            struct { Float4 translation; };
                        };
                    };

                    const struct
                    {
                        float _11, _12, _13, _14;
                        float _21, _22, _23, _24;
                        float _31, _32, _33, _34;
                        float _41, _42, _43, _44;
                    };
                };

            public:
                inline static Float4x4 FromScale(const Float3 &scale, const Float3 &translation = Math::Float3::Zero)
                {
                    return Float4x4(
                        scale.x, 0.0f, 0.0f, 0.0f,
                        0.0f, scale.y, 0.0f, 0.0f,
                        0.0f, 0.0f, scale.z, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                inline static Float4x4 FromAngular(const Float3 &axis, float radians, const Float3 &translation = Math::Float3::Zero)
                {
                    // do the trig
                    float cosAngle = cos(radians);
                    float sinAngle = sin(radians);
                    float theta = (1.0f - cosAngle);

                    // build the rotation matrix
                    return Float4x4(
                        theta*axis.x*axis.x + cosAngle, theta*axis.x*axis.y + sinAngle*axis.z, theta*axis.x*axis.z - sinAngle*axis.y, 0.0f,
                        theta*axis.x*axis.y - sinAngle*axis.z, theta*axis.y*axis.y + cosAngle, theta*axis.y*axis.z + sinAngle*axis.x, 0.0f,
                        theta*axis.x*axis.z + sinAngle*axis.y, theta*axis.y*axis.z - sinAngle*axis.x, theta*axis.z*axis.z + cosAngle, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                inline static Float4x4 FromEuler(float pitch, float yaw, float roll, const Float3 &translation = Math::Float3::Zero)
                {
                    float cosPitch(std::cos(pitch));
                    float sinPitch(std::sin(pitch));
                    float cosYaw(std::cos(yaw));
                    float sinYaw(std::sin(yaw));
                    float cosRoll(std::cos(roll));
                    float sinRoll(std::sin(roll));

                    return Float4x4(
                        cosYaw * cosRoll, cosYaw * sinRoll, -sinYaw, 0.0f,
                        cosRoll * sinPitch * sinYaw - cosPitch * sinRoll, cosPitch * cosRoll + sinPitch * sinYaw * sinRoll, cosYaw * sinPitch, 0.0f,
                        cosPitch * cosRoll * sinYaw + sinPitch * sinRoll, cosPitch * sinYaw * sinRoll - cosRoll * sinPitch, cosPitch * cosYaw, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                inline static Float4x4 FromPitch(float radians, const Float3 &translation = Math::Float3::Zero)
                {
                    float cosAngle(std::cos(radians));
                    float sinAngle(std::sin(radians));

                    return Float4x4(
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, cosAngle, sinAngle, 0.0f,
                        0.0f, -sinAngle, cosAngle, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                inline static Float4x4 FromYaw(float radians, const Float3 &translation = Math::Float3::Zero)
                {
                    float cosAngle(std::cos(radians));
                    float sinAngle(std::sin(radians));

                    return Float4x4(
                        cosAngle, 0.0f, -sinAngle, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        sinAngle, 0.0f, cosAngle, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                inline static Float4x4 FromRoll(float radians, const Float3 &translation = Math::Float3::Zero)
                {
                    float cosAngle(std::cos(radians));
                    float sinAngle(std::sin(radians));

                    return Float4x4(
                        cosAngle, sinAngle, 0.0f, 0.0f,
                        -sinAngle, cosAngle, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
                inline static Float4x4 MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip)
                {
                    return Float4x4(
                        (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                        0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                        0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f,
                        ((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f);
                }

                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
                inline static Float4x4 MakePerspective(float fieldOfView, float aspectRatio, float nearClip, float farClip)
                {
                    float yScale(1.0f / std::tan(fieldOfView * 0.5f));
                    float xScale(yScale / aspectRatio);
                    float denominator(farClip - nearClip);

                    return Float4x4(
                        xScale, 0.0f, 0.0f, 0.0f,
                        0.0f, yScale, 0.0f, 0.0f,
                        0.0f, 0.0f, (farClip / denominator), 1.0f,
                        0.0f, 0.0f, ((-nearClip * farClip) / denominator), 0.0f);
                }

                inline static Float4x4 MakeTargeted(const Float3 &source, const Float3 &target, const Float3 &worldUpVector, const Float3 &translation = Math::Float3::Zero)
                {
                    Float3 forward((target - source).getNormal());
                    Float3 left(worldUpVector.cross(forward).getNormal());
                    Float3 up(forward.cross(left));
                    return Float4x4(
                        left.x, left.y, left.z, 0.0f,
                        up.x, up.y, up.z, 0.0f,
                        forward.x, forward.y, forward.z, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }

            public:
                inline Float4x4(void)
                {
                }

                inline Float4x4(
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

                inline Float4x4(const float *data)
                    : simd{
                        _mm_loadu_ps(data + 0),
                        _mm_loadu_ps(data + 4),
                        _mm_loadu_ps(data + 8),
                        _mm_loadu_ps(data + 12) }
                {
                }

                inline Float4x4(
                    const __m128 &rx, 
                    const __m128 &ry,
                    const __m128 &rz,
                    const __m128 &rw)
                    : simd{ rx, ry, rz, rw }
                {
                }

                inline Float4x4(const Float4x4 &matrix)
                    : simd{ matrix.simd[0], matrix.simd[1], matrix.simd[2], matrix.simd[3] }
                {
                }

                inline Float4x4(const Quaternion &rotation, const Float3 &translation)
                    : rw(translation, 1.0f)
                {
                    setRotation(rotation);
                }

                inline void setRotation(const Quaternion &rotation)
                {
                    float xx(rotation.x * rotation.x);
                    float yy(rotation.y * rotation.y);
                    float zz(rotation.z * rotation.z);
                    float ww(rotation.w * rotation.w);
                    float length(xx + yy + zz + ww);
                    if (length == 0.0f)
                    {
                        rx.set(1.0f, 0.0f, 0.0f, 0.0f);
                        ry.set(0.0f, 1.0f, 0.0f, 0.0f);
                        rz.set(0.0f, 0.0f, 1.0f, 0.0f);
                    }
                    else
                    {
                        float determinant(1.0f / length);
                        float xy(rotation.x * rotation.y);
                        float xz(rotation.x * rotation.z);
                        float xw(rotation.x * rotation.w);
                        float yz(rotation.y * rotation.z);
                        float yw(rotation.y * rotation.w);
                        float zw(rotation.z * rotation.w);
                        rx.set(((xx - yy - zz + ww) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f);
                        ry.set((2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant), 0.0f);
                        rz.set((2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-xx - yy + zz + ww) * determinant), 0.0f);
                    }
                }

                inline Float3 getScaling(void) const
                {
                    return Float3(_11, _22, _33);
                }

                inline float getDeterminant(void) const
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

                inline Float4x4 getTranspose(void) const
                {
                    __m128 tmp3, tmp2, tmp1, tmp0;
                    tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
                    tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
                    tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
                    tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
                    return Float4x4(
                        _mm_shuffle_ps(tmp0, tmp1, 0x88),
                        _mm_shuffle_ps(tmp0, tmp1, 0xDD),
                        _mm_shuffle_ps(tmp2, tmp3, 0x88),
                        _mm_shuffle_ps(tmp2, tmp3, 0xDD));
                }

                inline Float4x4 getInverse(void) const
                {
                    float determinant(getDeterminant());
                    if (std::abs(determinant) < Epsilon)
                    {
                        return Identity;
                    }
                    else
                    {
                        determinant = (1.0f / determinant);

                        return Float4x4(
                            (determinant * (table[1][1] * (table[2][2] * table[3][3] - table[3][2] * table[2][3]) + table[2][1] * (table[3][2] * table[1][3] - table[1][2] * table[3][3]) + table[3][1] * (table[1][2] * table[2][3] - table[2][2] * table[1][3]))),
                            (determinant * (table[2][1] * (table[0][2] * table[3][3] - table[3][2] * table[0][3]) + table[3][1] * (table[2][2] * table[0][3] - table[0][2] * table[2][3]) + table[0][1] * (table[3][2] * table[2][3] - table[2][2] * table[3][3]))),
                            (determinant * (table[3][1] * (table[0][2] * table[1][3] - table[1][2] * table[0][3]) + table[0][1] * (table[1][2] * table[3][3] - table[3][2] * table[1][3]) + table[1][1] * (table[3][2] * table[0][3] - table[0][2] * table[3][3]))),
                            (determinant * (table[0][1] * (table[2][2] * table[1][3] - table[1][2] * table[2][3]) + table[1][1] * (table[0][2] * table[2][3] - table[2][2] * table[0][3]) + table[2][1] * (table[1][2] * table[0][3] - table[0][2] * table[1][3]))),
                            (determinant * (table[1][2] * (table[2][0] * table[3][3] - table[3][0] * table[2][3]) + table[2][2] * (table[3][0] * table[1][3] - table[1][0] * table[3][3]) + table[3][2] * (table[1][0] * table[2][3] - table[2][0] * table[1][3]))),
                            (determinant * (table[2][2] * (table[0][0] * table[3][3] - table[3][0] * table[0][3]) + table[3][2] * (table[2][0] * table[0][3] - table[0][0] * table[2][3]) + table[0][2] * (table[3][0] * table[2][3] - table[2][0] * table[3][3]))),
                            (determinant * (table[3][2] * (table[0][0] * table[1][3] - table[1][0] * table[0][3]) + table[0][2] * (table[1][0] * table[3][3] - table[3][0] * table[1][3]) + table[1][2] * (table[3][0] * table[0][3] - table[0][0] * table[3][3]))),
                            (determinant * (table[0][2] * (table[2][0] * table[1][3] - table[1][0] * table[2][3]) + table[1][2] * (table[0][0] * table[2][3] - table[2][0] * table[0][3]) + table[2][2] * (table[1][0] * table[0][3] - table[0][0] * table[1][3]))),
                            (determinant * (table[1][3] * (table[2][0] * table[3][1] - table[3][0] * table[2][1]) + table[2][3] * (table[3][0] * table[1][1] - table[1][0] * table[3][1]) + table[3][3] * (table[1][0] * table[2][1] - table[2][0] * table[1][1]))),
                            (determinant * (table[2][3] * (table[0][0] * table[3][1] - table[3][0] * table[0][1]) + table[3][3] * (table[2][0] * table[0][1] - table[0][0] * table[2][1]) + table[0][3] * (table[3][0] * table[2][1] - table[2][0] * table[3][1]))),
                            (determinant * (table[3][3] * (table[0][0] * table[1][1] - table[1][0] * table[0][1]) + table[0][3] * (table[1][0] * table[3][1] - table[3][0] * table[1][1]) + table[1][3] * (table[3][0] * table[0][1] - table[0][0] * table[3][1]))),
                            (determinant * (table[0][3] * (table[2][0] * table[1][1] - table[1][0] * table[2][1]) + table[1][3] * (table[0][0] * table[2][1] - table[2][0] * table[0][1]) + table[2][3] * (table[1][0] * table[0][1] - table[0][0] * table[1][1]))),
                            (determinant * (table[1][0] * (table[3][1] * table[2][2] - table[2][1] * table[3][2]) + table[2][0] * (table[1][1] * table[3][2] - table[3][1] * table[1][2]) + table[3][0] * (table[2][1] * table[1][2] - table[1][1] * table[2][2]))),
                            (determinant * (table[2][0] * (table[3][1] * table[0][2] - table[0][1] * table[3][2]) + table[3][0] * (table[0][1] * table[2][2] - table[2][1] * table[0][2]) + table[0][0] * (table[2][1] * table[3][2] - table[3][1] * table[2][2]))),
                            (determinant * (table[3][0] * (table[1][1] * table[0][2] - table[0][1] * table[1][2]) + table[0][0] * (table[3][1] * table[1][2] - table[1][1] * table[3][2]) + table[1][0] * (table[0][1] * table[3][2] - table[3][1] * table[0][2]))),
                            (determinant * (table[0][0] * (table[1][1] * table[2][2] - table[2][1] * table[1][2]) + table[1][0] * (table[2][1] * table[0][2] - table[0][1] * table[2][2]) + table[2][0] * (table[0][1] * table[1][2] - table[1][1] * table[0][2]))));
                    }
                }

                inline Quaternion getRotation(void) const
                {
                    float trace(table[0][0] + table[1][1] + table[2][2] + 1.0f);
                    if (trace > Epsilon)
                    {
                        float denominator(0.5f / std::sqrt(trace));
                        return Quaternion(
                            ((table[1][2] - table[2][1]) * denominator),
                            ((table[2][0] - table[0][2]) * denominator),
                            ((table[0][1] - table[1][0]) * denominator),
                            (0.25f / denominator));
                    }
                    else
                    {
                        if ((table[0][0] > table[1][1]) && (table[0][0] > table[2][2]))
                        {
                            float denominator(2.0f * std::sqrt(1.0f + table[0][0] - table[1][1] - table[2][2]));
                            return Quaternion(
                                (0.25f * denominator),
                                ((table[1][0] + table[0][1]) / denominator),
                                ((table[2][0] + table[0][2]) / denominator),
                                ((table[2][1] - table[1][2]) / denominator));
                        }
                        else if (table[1][1] > table[2][2])
                        {
                            float denominator(2.0f * (std::sqrt(1.0f + table[1][1] - table[0][0] - table[2][2])));
                            return Quaternion(
                                ((table[1][0] + table[0][1]) / denominator),
                                (0.25f * denominator),
                                ((table[2][1] + table[1][2]) / denominator),
                                ((table[2][0] - table[0][2]) / denominator));
                        }
                        else
                        {
                            float denominator(2.0f * (std::sqrt(1.0f + table[2][2] - table[0][0] - table[1][1])));
                            return Quaternion(
                                ((table[2][0] + table[0][2]) / denominator),
                                ((table[2][1] + table[1][2]) / denominator),
                                (0.25f * denominator),
                                ((table[1][0] - table[0][1]) / denominator));
                        }
                    }
                }

                inline Float4x4 &transpose(void)
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

                inline Float4x4 &invert(void)
                {
                    (*this) = getInverse();
                    return (*this);
                }

                inline Float4 operator [] (int row) const
                {
                    return rows[row];
                }

                inline Float4 &operator [] (int row)
                {
                    return rows[row];
                }

                inline operator const __m128 *() const
                {
                    return simd;
                }

                inline operator const float *() const
                {
                    return data;
                }

                inline Float4x4 &operator = (const Float4x4 &matrix)
                {
                    rows[0] = matrix.rows[0];
                    rows[1] = matrix.rows[1];
                    rows[2] = matrix.rows[2];
                    rows[3] = matrix.rows[3];
                    return (*this);
                }

                inline void operator *= (const Float4x4 &matrix)
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

                inline Float4x4 operator * (const Float4x4 &matrix) const
                {
                    return Float4x4(
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))));
                }

                inline Float3 rotate(const Float3 &vector) const
                {
                    return Float3(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)));
                }

                inline Float3 transform(const Float3 &vector) const
                {
                    return Float3(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + _41),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + _42),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + _43));
                }

                inline Float4 transform(const Float4 &vector) const
                {
                    return Float4(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                        ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)));
                }
            };
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

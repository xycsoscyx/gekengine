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
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include <xmmintrin.h>
#include <tuple>

namespace Gek
{
    namespace Math
    {
        struct Float4x4
        {
        public:
            static const Float4x4 Identity;

        public:
            union
            {
                float data[16];
                float table[4][4];
                struct
                {
                    float _11, _12, _13, _14;
                    float _21, _22, _23, _24;
                    float _31, _32, _33, _34;
                    float _41, _42, _43, _44;
                };

                Float4 rows[4];
                struct
                {
                    Float4 x, y, z, w;
                } r;
            };

        public:
            inline static Float4x4 MakeScaling(Float3 const &scale, Float3 const &translation = Math::Float3::Zero) noexcept
            {
                return Float4x4(
                    scale.x, 0.0f, 0.0f, 0.0f,
                    0.0f, scale.y, 0.0f, 0.0f,
                    0.0f, 0.0f, scale.z, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            inline static Float4x4 MakeQuaternionRotation(Quaternion const &rotation, Float3 const &translation = Math::Float3::Zero) noexcept
            {
                Float4x4 matrix;
                matrix.setRotation(rotation);
                matrix.r.w = Float4(translation, 1.0f);
                return matrix;
            }

            inline static Float4x4 MakeAngularRotation(Float3 const &axis, float radians, Float3 const &translation = Math::Float3::Zero) noexcept
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

            inline static Float4x4 MakeEulerRotation(float pitch, float yaw, float roll, Float3 const &translation = Math::Float3::Zero) noexcept
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

            inline static Float4x4 MakePitchRotation(float radians, Float3 const &translation = Math::Float3::Zero) noexcept
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return Float4x4(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cosAngle, sinAngle, 0.0f,
                    0.0f, -sinAngle, cosAngle, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            inline static Float4x4 MakeYawRotation(float radians, Float3 const &translation = Math::Float3::Zero) noexcept
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return Float4x4(
                    cosAngle, 0.0f, -sinAngle, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    sinAngle, 0.0f, cosAngle, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            inline static Float4x4 MakeRollRotation(float radians, Float3 const &translation = Math::Float3::Zero) noexcept
            {
                float cosAngle(std::cos(radians));
                float sinAngle(std::sin(radians));

                return Float4x4(
                    cosAngle, sinAngle, 0.0f, 0.0f,
                    -sinAngle, cosAngle, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            inline static Float4x4 MakeTranslation(Float3 const &translation) noexcept
            {
                return Float4x4(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    translation.x, translation.y, translation.z, 1.0f);
            }

            // https://msdn.microsoft.com/en-us/librar.y/windows/desktop/bb205347(v=vs.85).aspx
            inline static Float4x4 MakeOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip) noexcept
            {
                return Float4x4(
                    (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                    0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                    0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f,
                    ((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f);
            }

            // https://msdn.microsoft.com/en-us/librar.y/windows/desktop/bb205350(v=vs.85).aspx
            inline static Float4x4 MakePerspective(float fieldOfView, float aspectRatio, float nearClip, float farClip) noexcept
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

        public:
            inline Float4x4(void) noexcept
            {
            }

            inline Float4x4(Float4x4 const &matrix) noexcept
                : rows {
                    matrix.rows[0],
                    matrix.rows[1],
                    matrix.rows[2],
                    matrix.rows[3] }
            {
            }

            explicit inline Float4x4(
                float _11, float _12, float _13, float _14,
                float _21, float _22, float _23, float _24,
                float _31, float _32, float _33, float _34,
                float _41, float _42, float _43, float _44) noexcept
                : _11(_11), _12(_12), _13(_13), _14(_14)
                , _21(_21), _22(_22), _23(_23), _24(_24)
                , _31(_31), _32(_32), _33(_33), _34(_34)
                , _41(_41), _42(_42), _43(_43), _44(_44)
            {
            }

            explicit inline Float4x4(float const *data) noexcept
                : rows {
                    Float4(data + 0),
                    Float4(data + 4),
                    Float4(data + 8),
                    Float4(data + 12) }
            {
            }

            inline void setRotation(Quaternion const &rotation) noexcept
            {
                float xx(rotation.x * rotation.x);
                float yy(rotation.y * rotation.y);
                float zz(rotation.z * rotation.z);
                float ww(rotation.w * rotation.w);
                float length(xx + yy + zz + ww);
                if (length == 0.0f)
                {
                    r.x.set(1.0f, 0.0f, 0.0f, 0.0f);
                    r.y.set(0.0f, 1.0f, 0.0f, 0.0f);
                    r.z.set(0.0f, 0.0f, 1.0f, 0.0f);
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
                    r.x.set(((xx - yy - zz + ww) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f);
                    r.y.set((2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant), 0.0f);
                    r.z.set((2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-xx - yy + zz + ww) * determinant), 0.0f);
                }
            }

            Float3& translation(void)
            {
                return r.w.xyz();
            }

            const Float3& translation(void) const
            {
                return r.w.xyz();
            }

            inline float getDeterminant(void) const noexcept
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

            inline Float4x4 getTranspose(void) const noexcept
            {
                return Float4x4(
                    _11, _21, _31, _41,
                    _12, _22, _32, _42,
                    _13, _23, _33, _43,
                    _14, _24, _34, _44);
            }

            inline Float4x4 getInverse(void) const noexcept
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

            inline Quaternion getRotation(void) const noexcept
            {
                Math::Float3 normalized[3] =
                {
                    r.x.xyz().getNormal(),
                    r.y.xyz().getNormal(),
                    r.z.xyz().getNormal(),
                };

                Quaternion result;
                float trace = normalized[0].data[0] + normalized[1].data[1] + normalized[2].data[2];
                if (trace > 0.0f)
                {
                    trace = std::sqrt(trace + 1.0f);
                    result.w = 0.5f * trace;
                    trace = 0.5f / trace;
                    result.x = (normalized[1].data[2] - normalized[2].data[1]) * trace;
                    result.y = (normalized[2].data[0] - normalized[0].data[2]) * trace;
                    result.z = (normalized[0].data[1] - normalized[1].data[0]) * trace;
                }
                else
                {
                    enum QuaternionAxis
                    {
                        X_INDEX = 0,
                        Y_INDEX = 1,
                        Z_INDEX = 2
                    };

                    static QuaternionAxis NextAxisList[] =
                    {
                        Y_INDEX,
                        Z_INDEX,
                        X_INDEX
                    };

                    QuaternionAxis localXAxis = X_INDEX;
                    if (normalized[Y_INDEX].data[Y_INDEX] > normalized[X_INDEX].data[X_INDEX])
                    {
                        localXAxis = Y_INDEX;
                    }
                    
                    if (normalized[Z_INDEX].data[Z_INDEX] > normalized[localXAxis].data[localXAxis])
                    {
                        localXAxis = Z_INDEX;
                    }

                    QuaternionAxis localYAxis = NextAxisList[localXAxis];
                    QuaternionAxis localZAxis = NextAxisList[localYAxis];
                    trace = 1.0f + normalized[localXAxis].data[localXAxis] - normalized[localYAxis].data[localYAxis] - normalized[localZAxis].data[localZAxis];
                    trace = std::sqrt(trace);

                    result.data[localXAxis] = 0.5f * trace;
                    trace = 0.5f / trace;
                    result.w = (normalized[localYAxis].data[localZAxis] - normalized[localZAxis].data[localYAxis]) * trace;
                    result.data[localYAxis] = (normalized[localXAxis].data[localYAxis] + normalized[localYAxis].data[localXAxis]) * trace;
                    result.data[localZAxis] = (normalized[localXAxis].data[localZAxis] + normalized[localZAxis].data[localXAxis]) * trace;
                }

                return result;
            }

            inline Float3 getScaling(void) const noexcept
            {
                return Math::Float3(
                    r.x.xyz().getLength(),
                    r.y.xyz().getLength(),
                    r.z.xyz().getLength());
            }

            inline Float4x4 &transpose(void) noexcept
            {
                (*this) = getTranspose();
                return (*this);
            }

            inline Float4x4 &invert(void) noexcept
            {
                (*this) = getInverse();
                return (*this);
            }

            inline Float4x4 &orthonormalize(void) noexcept
            {
                r.x = Float4(r.x.xyz().getNormal(), 0.0f);
                r.y = Float4(r.y.xyz().getNormal(), 0.0f);
                r.z = Float4(r.z.xyz().getNormal(), 0.0f);
                return (*this);
            }

            inline void operator *= (Float4x4 const &matrix) noexcept
            {
                __m128 simd[2][4] =
                {
                    {
                        _mm_loadu_ps(r.x.data),
                        _mm_loadu_ps(r.y.data),
                        _mm_loadu_ps(r.z.data),
                        _mm_loadu_ps(r.w.data),
                    },
                    {
                        _mm_loadu_ps(matrix.r.x.data),
                        _mm_loadu_ps(matrix.r.y.data),
                        _mm_loadu_ps(matrix.r.z.data),
                        _mm_loadu_ps(matrix.r.w.data),
                    },
                };

                _mm_storeu_ps(r.x.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(r.y.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(r.z.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(r.w.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
            }

            inline Float4x4 operator * (Float4x4 const &matrix) const noexcept
            {
                __m128 simd[2][4] =
                {
                    {
                        _mm_loadu_ps(r.x.data),
                        _mm_loadu_ps(r.y.data),
                        _mm_loadu_ps(r.z.data),
                        _mm_loadu_ps(r.w.data),
                    },
                    {
                        _mm_loadu_ps(matrix.r.x.data),
                        _mm_loadu_ps(matrix.r.y.data),
                        _mm_loadu_ps(matrix.r.z.data),
                        _mm_loadu_ps(matrix.r.w.data),
                    },
                };

                Float4x4 result;
                _mm_storeu_ps(result.r.x.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.r.y.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.r.z.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.r.w.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                return result;
            }

            inline Float3 rotate(Float3 const &vector) const noexcept
            {
                return Float3(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)));
            }

            inline Float3 transform(Float3 const &vector) const noexcept
            {
                return Float3(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + _41),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + _42),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + _43));
            }

            inline Float4 transform(Float4 const &vector) const noexcept
            {
                return Float4(
                    ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                    ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                    ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                    ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)));
            }

            std::tuple<Float4, Float4, Float4, Float4> getTuple(void) const noexcept
            {
                return std::make_tuple(r.x, r.y, r.z, r.w);
            }

            bool operator == (Float4x4 const &matrix) const noexcept
            {
                return (getTuple() == matrix.getTuple());
            }

            bool operator != (Float4x4 const &matrix) const noexcept
            {
                return (getTuple() != matrix.getTuple());
            }

            Float4 &operator [] (size_t index) noexcept
            {
                return rows[index];
            }

            Float4 const &operator [] (size_t index) const noexcept
            {
                return rows[index];
            }

            inline Float4x4 &operator = (Float4x4 const &matrix) noexcept
            {
                std::tie(r.x, r.y, r.z, r.w) = matrix.getTuple();
                return (*this);
            }
        };
    }; // namespace Math
}; // namespace Gek

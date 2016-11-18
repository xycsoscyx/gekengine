/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK\Math\Common.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\Quaternion.hpp"
#include <xmmintrin.h>

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
                : _11(_11), _12(_12), _13(_13), _14(_14)
                , _21(_21), _22(_22), _23(_23), _24(_24)
                , _31(_31), _32(_32), _33(_33), _34(_34)
                , _41(_41), _42(_42), _43(_43), _44(_44)
            {
            }

            inline Float4x4(const float *data)
                : rx(data + 0)
                , ry(data + 4)
                , rz(data + 8)
                , rw(data + 12)
            {
            }

            inline Float4x4(const Float4x4 &matrix)
                : rx(matrix.data + 0)
                , ry(matrix.data + 4)
                , rz(matrix.data + 8)
                , rw(matrix.data + 12)
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
                return Float4x4(
                    _11, _21, _31, _41,
                    _12, _22, _32, _42,
                    _13, _23, _33, _43,
                    _14, _24, _34, _44);
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

                Quaternion result;
                float trace = table[0][0] + table[1][1] + table[2][2];
                if (trace > 0.0f)
                {
                    trace = std::sqrt(trace + 1.0f);
                    result.w = 0.5f * trace;
                    trace = 0.5f / trace;
                    result.x = (table[1][2] - table[2][1]) * trace;
                    result.y = (table[2][0] - table[0][2]) * trace;
                    result.z = (table[0][1] - table[1][0]) * trace;
                }
                else
                {
                    QuaternionAxis localXAxis = X_INDEX;
                    if (table[Y_INDEX][Y_INDEX] > table[X_INDEX][X_INDEX])
                    {
                        localXAxis = Y_INDEX;
                    }
                    
                    if (table[Z_INDEX][Z_INDEX] > table[localXAxis][localXAxis])
                    {
                        localXAxis = Z_INDEX;
                    }

                    QuaternionAxis localYAxis = NextAxisList[localXAxis];
                    QuaternionAxis localZAxis = NextAxisList[localYAxis];
                    trace = 1.0f + table[localXAxis][localXAxis] - table[localYAxis][localYAxis] - table[localZAxis][localZAxis];
                    trace = std::sqrt(trace);

                    result.axis[localXAxis] = 0.5f * trace;
                    trace = 0.5f / trace;
                    result.w = (table[localYAxis][localZAxis] - table[localZAxis][localYAxis]) * trace;
                    result.axis[localYAxis] = (table[localXAxis][localYAxis] + table[localYAxis][localXAxis]) * trace;
                    result.axis[localZAxis] = (table[localXAxis][localZAxis] + table[localZAxis][localXAxis]) * trace;
                }

                return result;
            }

            inline Float4x4 &transpose(void)
            {
                (*this) = getTranspose();
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
                __m128 simd[2][4] =
                {
                    {
                        _mm_loadu_ps(rx.data),
                        _mm_loadu_ps(ry.data),
                        _mm_loadu_ps(rz.data),
                        _mm_loadu_ps(rw.data),
                    },
                    {
                        _mm_loadu_ps(matrix.rx.data),
                        _mm_loadu_ps(matrix.ry.data),
                        _mm_loadu_ps(matrix.rz.data),
                        _mm_loadu_ps(matrix.rw.data),
                    },
                };

                _mm_storeu_ps(rx.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(ry.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(rz.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(rw.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
            }

            inline Float4x4 operator * (const Float4x4 &matrix) const
            {
                __m128 simd[2][4] =
                {
                    {
                        _mm_loadu_ps(rx.data),
                        _mm_loadu_ps(ry.data),
                        _mm_loadu_ps(rz.data),
                        _mm_loadu_ps(rw.data),
                    },
                    {
                        _mm_loadu_ps(matrix.rx.data),
                        _mm_loadu_ps(matrix.ry.data),
                        _mm_loadu_ps(matrix.rz.data),
                        _mm_loadu_ps(matrix.rw.data),
                    },
                };

                Float4x4 result;
                _mm_storeu_ps(result.rx.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][0], simd[0][0], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.ry.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][1], simd[0][1], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.rz.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][2], simd[0][2], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                _mm_storeu_ps(result.rw.data, _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(0, 0, 0, 0)), simd[1][0]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(1, 1, 1, 1)), simd[1][1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(2, 2, 2, 2)), simd[1][2]), _mm_mul_ps(_mm_shuffle_ps(simd[0][3], simd[0][3], _MM_SHUFFLE(3, 3, 3, 3)), simd[1][3]))));
                return result;
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
    }; // namespace Math
}; // namespace Gek

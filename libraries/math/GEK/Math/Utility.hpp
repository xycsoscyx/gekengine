/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Constants.hpp"
#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\Vector4SIMD.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Math\Quaternion.hpp"
#include <type_traits>

namespace Gek
{
    namespace Math
    {
        namespace Utility
        {
            template <typename TYPE>
            TYPE convertDegreesToRadians(TYPE degrees)
            {
                return TYPE(degrees * (Pi / 180.0f));
            }

            template <typename TYPE>
            TYPE convertRadiansToDegrees(TYPE radians)
            {
                return TYPE(radians * (180.0f / Pi));
            }

            template <typename DATA, typename TYPE>
            DATA lerp(const DATA &valueA, const DATA &valueB, TYPE factor)
            {
                return (((valueB - valueA) * factor) + valueA);
            }

            template <typename DATA, typename TYPE>
            DATA blend(const DATA &valueA, const DATA &valueB, TYPE factor)
            {
                return ((valueA * (1.0f - factor)) + (valueB * factor));
            }

            template <typename DATA, typename TYPE>
            DATA blend(const DATA &valueA, TYPE factorX, const DATA &valueB, TYPE factorY)
            {
                return ((valueA * factorX) + (valueB * factorY));
            }

            template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
            BaseQuaternion<TYPE> convert(const SIMD::Matrix4x4<TYPE> &matrix)
            {
                TYPE trace(matrix.table[0][0] + matrix.table[1][1] + matrix.table[2][2] + 1.0f);
                if (trace > Epsilon)
                {
                    TYPE denominator(0.5f / std::sqrt(trace));
                    return BaseQuaternion<TYPE>(
                        ((matrix.table[1][2] - matrix.table[2][1]) * denominator),
                        ((matrix.table[2][0] - matrix.table[0][2]) * denominator),
                        ((matrix.table[0][1] - matrix.table[1][0]) * denominator),
                        (0.25f / denominator));
                }
                else
                {
                    if ((matrix.table[0][0] > matrix.table[1][1]) && (matrix.table[0][0] > matrix.table[2][2]))
                    {
                        TYPE denominator(2.0f * std::sqrt(1.0f + matrix.table[0][0] - matrix.table[1][1] - matrix.table[2][2]));
                        return BaseQuaternion<TYPE>(
                            (0.25f * denominator),
                            ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                            ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                            ((matrix.table[2][1] - matrix.table[1][2]) / denominator));
                    }
                    else if (matrix.table[1][1] > matrix.table[2][2])
                    {
                        TYPE denominator(2.0f * (std::sqrt(1.0f + matrix.table[1][1] - matrix.table[0][0] - matrix.table[2][2])));
                        return BaseQuaternion<TYPE>(
                            ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                            (0.25f * denominator),
                            ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                            ((matrix.table[2][0] - matrix.table[0][2]) / denominator));
                    }
                    else
                    {
                        TYPE denominator(2.0f * (std::sqrt(1.0f + matrix.table[2][2] - matrix.table[0][0] - matrix.table[1][1])));
                        return BaseQuaternion<TYPE>(
                            ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                            ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                            (0.25f * denominator),
                            ((matrix.table[1][0] - matrix.table[0][1]) / denominator));
                    }
                }
            }

            template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
            SIMD::Matrix4x4<TYPE> convert(const BaseQuaternion<TYPE> &quaternion, const Vector3<TYPE> &translation = Math::Vector3<TYPE>::Zero)
            {
                TYPE xx(quaternion.x * quaternion.x);
                TYPE yy(quaternion.y * quaternion.y);
                TYPE zz(quaternion.z * quaternion.z);
                TYPE ww(quaternion.w * quaternion.w);
                TYPE length(xx + yy + zz + ww);
                if (length == 0.0f)
                {
                    return SIMD::Matrix4x4<TYPE>(
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }
                else
                {
                    TYPE determinant(1.0f / length);
                    TYPE xy(quaternion.x * quaternion.y);
                    TYPE xz(quaternion.x * quaternion.z);
                    TYPE xw(quaternion.x * quaternion.w);
                    TYPE yz(quaternion.y * quaternion.z);
                    TYPE yw(quaternion.y * quaternion.w);
                    TYPE zw(quaternion.z * quaternion.w);
                    return SIMD::Matrix4x4<TYPE>(
                        ((xx - yy - zz + ww) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f,
                        (2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant), 0.0f,
                        (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-xx - yy + zz + ww) * determinant), 0.0f,
                        translation.x, translation.y, translation.z, 1.0f);
                }
            }

            namespace Matrix
            {
                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createScaling(const Vector3<TYPE> &scale)
                {
                    return SIMD::Matrix4x4<TYPE>(
                        scale.x, 0.0f, 0.0f, 0.0f,
                        0.0f, scale.y, 0.0f, 0.0f,
                        0.0f, 0.0f, scale.z, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createEulerRotation(TYPE pitch, TYPE yaw, TYPE roll)
                {
                    TYPE cosPitch(std::cos(pitch));
                    TYPE sinPitch(std::sin(pitch));
                    TYPE cosYaw(std::cos(yaw));
                    TYPE sinYaw(std::sin(yaw));
                    TYPE cosRoll(std::cos(roll));
                    TYPE sinRoll(std::sin(roll));
                    TYPE cosPitchSinYaw(cosPitch * sinYaw);
                    TYPE sinPitchSinYaw(sinPitch * sinYaw);

                    return SIMD::Matrix4x4<TYPE>(
                        (cosYaw * cosRoll), (-cosYaw * sinRoll), sinYaw, 0.0f,
                        (sinPitchSinYaw * cosRoll + cosPitch * sinRoll), (-sinPitchSinYaw * sinRoll + cosPitch * cosRoll), (-sinPitch * cosYaw), 0.0f,
                        (-cosPitchSinYaw * cosRoll + sinPitch * sinRoll), (cosPitchSinYaw * sinRoll + sinPitch * cosRoll), (cosPitch * cosYaw), 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createAngularRotation(const Vector3<TYPE> &axis, TYPE radians)
                {
                    TYPE cosAngle(std::cos(radians));
                    TYPE sinAngle(std::sin(radians));

                    return SIMD::Matrix4x4<TYPE>(
                        (cosAngle + axis.x * axis.x * (1.0f - cosAngle)), (axis.z * sinAngle + axis.y * axis.x * (1.0f - cosAngle)), (-axis.y * sinAngle + axis.z * axis.x * (1.0f - cosAngle)), 0.0f,
                        (-axis.z * sinAngle + axis.x * axis.y * (1.0f - cosAngle)), (cosAngle + axis.y * axis.y * (1.0f - cosAngle)), (axis.x * sinAngle + axis.z * axis.y * (1.0f - cosAngle)), 0.0f,
                        (axis.y * sinAngle + axis.x * axis.z * (1.0f - cosAngle)), (-axis.x * sinAngle + axis.y * axis.z * (1.0f - cosAngle)), (cosAngle + axis.z * axis.z * (1.0f - cosAngle)), 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createPitchRotation(TYPE radians)
                {
                    TYPE cosAngle(std::cos(radians));
                    TYPE sinAngle(std::sin(radians));

                    return SIMD::Matrix4x4<TYPE>(
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, cosAngle, sinAngle, 0.0f,
                        0.0f, -sinAngle, cosAngle, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createYawRotation(TYPE radians)
                {
                    TYPE cosAngle(std::cos(radians));
                    TYPE sinAngle(std::sin(radians));

                    return SIMD::Matrix4x4<TYPE>(
                        cosAngle, 0.0f, -sinAngle, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        sinAngle, 0.0f, cosAngle, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createRollRotation(TYPE radians)
                {
                    TYPE cosAngle(std::cos(radians));
                    TYPE sinAngle(std::sin(radians));

                    return SIMD::Matrix4x4<TYPE>(
                        cosAngle, sinAngle, 0.0f, 0.0f,
                        -sinAngle, cosAngle, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createOrthographic(TYPE left, TYPE top, TYPE right, TYPE bottom, TYPE nearClip, TYPE farClip)
                {
                    return SIMD::Matrix4x4<TYPE>(
                        (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                        0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                        0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f,
                        ((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f);
                }

                // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createPerspective(TYPE fieldOfView, TYPE aspectRatio, TYPE nearClip, TYPE farClip)
                {
                    TYPE yScale(1.0f / std::tan(fieldOfView * 0.5f));
                    TYPE xScale(yScale / aspectRatio);
                    TYPE denominator(farClip - nearClip);

                    return SIMD::Matrix4x4<TYPE>(
                        xScale, 0.0f, 0.0f, 0.0f,
                        0.0f, yScale, 0.0f, 0.0f,
                        0.0f, 0.0f, (farClip / denominator), 1.0f,
                        0.0f, 0.0f, ((-nearClip * farClip) / denominator), 0.0f);
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                SIMD::Matrix4x4<TYPE> createLookAt(const Vector3<TYPE> &source, const Vector3<TYPE> &target, const Vector3<TYPE> &worldUpVector)
                {
                    Vector3<TYPE> forward((target - source).getNormal());
                    Vector3<TYPE> left(worldUpVector.cross(forward).getNormal());
                    Vector3<TYPE> up(forward.cross(left));
                    return SIMD::Matrix4x4<TYPE>(
                        left.x, left.y, left.z, 0.0f,
                        up.x, up.y, up.z, 0.0f,
                        forward.x, forward.y, forward.z, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }
            }; // namespace Matrix

            namespace Quaternion
            {
                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                BaseQuaternion<TYPE> createEulerRotation(TYPE pitch, TYPE yaw, TYPE roll)
                {
                    TYPE sinPitch(std::sin(pitch * 0.5f));
                    TYPE sinYaw(std::sin(yaw * 0.5f));
                    TYPE sinRoll(std::sin(roll * 0.5f));
                    TYPE cosPitch(std::cos(pitch * 0.5f));
                    TYPE cosYaw(std::cos(yaw * 0.5f));
                    TYPE cosRoll(std::cos(roll * 0.5f));
                    return BaseQuaternion<TYPE>(
                        ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                        ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                        ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                        ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)));
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                BaseQuaternion<TYPE> createAngularRotation(const Vector3<TYPE> &axis, TYPE radians)
                {
                    TYPE halfRadians = (radians * 0.5f);
                    Vector3<TYPE> normal(axis.getNormal());
                    TYPE sinAngle(std::sin(halfRadians));
                    return BaseQuaternion<TYPE>(
                        (normal.x * sinAngle),
                        (normal.y * sinAngle),
                        (normal.z * sinAngle),
                        std::cos(halfRadians));
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                BaseQuaternion<TYPE> createPitchRotation(TYPE radians)
                {
                    TYPE halfRadians = (radians * 0.5f);
                    TYPE sinAngle(std::sin(halfRadians));
                    return BaseQuaternion<TYPE>(
                        sinAngle,
                        0.0f,
                        0.0f,
                        std::cos(halfRadians));
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                BaseQuaternion<TYPE> createYawRotation(TYPE radians)
                {
                    TYPE halfRadians = (radians * 0.5f);
                    TYPE sinAngle(std::sin(halfRadians));
                    return BaseQuaternion<TYPE>(
                        0.0f,
                        sinAngle,
                        0.0f,
                        std::cos(halfRadians));
                }

                template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
                BaseQuaternion<TYPE> createRollRotation(TYPE radians)
                {
                    TYPE halfRadians = (radians * 0.5f);
                    TYPE sinAngle(std::sin(halfRadians));
                    return BaseQuaternion<TYPE>(
                        0.0f,
                        0.0f,
                        sinAngle,
                        std::cos(halfRadians));
                }
            }; // namespace Quaternion
        }; // namespace Utility
    }; // namespace Math
}; // namespace Gek

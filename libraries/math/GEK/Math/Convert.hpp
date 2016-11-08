/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Common.hpp"
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
        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        SIMD::Matrix4x4<TYPE> convert(const Quaternion<TYPE> &quaternion, const Vector3<TYPE> &translation = Math::Vector3<TYPE>::Zero)
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

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        Quaternion<TYPE> convert(const SIMD::Matrix4x4<TYPE> &matrix)
        {
            TYPE trace(matrix.table[0][0] + matrix.table[1][1] + matrix.table[2][2] + 1.0f);
            if (trace > Epsilon)
            {
                TYPE denominator(0.5f / std::sqrt(trace));
                return Quaternion<TYPE>(
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
                    return Quaternion<TYPE>(
                        (0.25f * denominator),
                        ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                        ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                        ((matrix.table[2][1] - matrix.table[1][2]) / denominator));
                }
                else if (matrix.table[1][1] > matrix.table[2][2])
                {
                    TYPE denominator(2.0f * (std::sqrt(1.0f + matrix.table[1][1] - matrix.table[0][0] - matrix.table[2][2])));
                    return Quaternion<TYPE>(
                        ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                        (0.25f * denominator),
                        ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                        ((matrix.table[2][0] - matrix.table[0][2]) / denominator));
                }
                else
                {
                    TYPE denominator(2.0f * (std::sqrt(1.0f + matrix.table[2][2] - matrix.table[0][0] - matrix.table[1][1])));
                    return Quaternion<TYPE>(
                        ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                        ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                        (0.25f * denominator),
                        ((matrix.table[1][0] - matrix.table[0][1]) / denominator));
                }
            }
        }
	}; // namespace Math
}; // namespace Gek

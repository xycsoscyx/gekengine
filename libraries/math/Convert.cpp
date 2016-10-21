#include "GEK\Math\Quaternion.hpp"
#include "GEK\Math\Matrix4x4.hpp"
#include "GEK\Math\Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float4x4 convert(const Quaternion &quaternion)
        {
            float xx(quaternion.x * quaternion.x);
            float yy(quaternion.y * quaternion.y);
            float zz(quaternion.z * quaternion.z);
            float ww(quaternion.w * quaternion.w);
            float length(xx + yy + zz + ww);
            if (length == 0.0f)
            {
                return Float4x4::Identity;
            }
            else
            {
                float determinant(1.0f / length);
                float xy(quaternion.x * quaternion.y);
                float xz(quaternion.x * quaternion.z);
                float xw(quaternion.x * quaternion.w);
                float yz(quaternion.y * quaternion.z);
                float yw(quaternion.y * quaternion.w);
                float zw(quaternion.z * quaternion.w);
                return Float4x4({ ((xx - yy - zz + ww) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f,
                                   (2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant), 0.0f,
                                   (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-xx - yy + zz + ww) * determinant), 0.0f,
                                    0.0f, 0.0f, 0.0f, 1.0f });
            }
        }

        Quaternion convert(const Float4x4 &matrix)
        {
            float trace(matrix.table[0][0] + matrix.table[1][1] + matrix.table[2][2] + 1.0f);
            if (trace > Epsilon)
            {
                float denominator(0.5f / std::sqrt(trace));
                return Quaternion(
                {
                    ((matrix.table[1][2] - matrix.table[2][1]) * denominator),
                    ((matrix.table[2][0] - matrix.table[0][2]) * denominator),
                    ((matrix.table[0][1] - matrix.table[1][0]) * denominator),
                    (0.25f / denominator),
                });
            }
            else
            {
                if ((matrix.table[0][0] > matrix.table[1][1]) && (matrix.table[0][0] > matrix.table[2][2]))
                {
                    float denominator(2.0f * std::sqrt(1.0f + matrix.table[0][0] - matrix.table[1][1] - matrix.table[2][2]));
                    return Quaternion(
                    {
                        (0.25f * denominator),
                        ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                        ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                        ((matrix.table[2][1] - matrix.table[1][2]) / denominator),
                    });
                }
                else if (matrix.table[1][1] > matrix.table[2][2])
                {
                    float denominator(2.0f * (std::sqrt(1.0f + matrix.table[1][1] - matrix.table[0][0] - matrix.table[2][2])));
                    return Quaternion(
                    {
                        ((matrix.table[1][0] + matrix.table[0][1]) / denominator),
                        (0.25f * denominator),
                        ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                        ((matrix.table[2][0] - matrix.table[0][2]) / denominator),
                    });
                }
                else
                {
                    float denominator(2.0f * (std::sqrt(1.0f + matrix.table[2][2] - matrix.table[0][0] - matrix.table[1][1])));
                    return Quaternion(
                    {
                        ((matrix.table[2][0] + matrix.table[0][2]) / denominator),
                        ((matrix.table[2][1] + matrix.table[1][2]) / denominator),
                        (0.25f * denominator),
                        ((matrix.table[1][0] - matrix.table[0][1]) / denominator),
                    });
                }
            }
        }
    }; // namespace Math
}; // namespace Gek

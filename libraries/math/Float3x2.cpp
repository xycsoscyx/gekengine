#include "GEK\Math\Float3x2.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Float3x2 Float3x2::Identity({ 1.0f, 0.0f,
                                            0.0f, 1.0f,
                                            0.0f, 0.0f });

        Float3x2::Float3x2(const float(&data)[6])
            : data{ data[0], data[1],
                    data[2], data[3],
                    data[4], data[5] }
        {
        }

        Float3x2::Float3x2(const float *data)
            : data{ data[0], data[1],
                    data[2], data[3],
                    data[4], data[5] }
        {
        }

        Float3x2::Float3x2(const Float3x2 &matrix)
            : rows{ matrix.rows[0],
                    matrix.rows[1],
                    matrix.rows[2] }
        {
        }

        Float3x2 Float3x2::setScaling(float scale)
        {
            return Float3x2(
            {
                scale, 0.0f,
                0.0f, scale,
                0.0f, 0.0f,
            });
        }

        Float3x2 Float3x2::setScaling(const Float2 &scale)
        {
            return Float3x2(
            {
                scale.x, 0.0f,
                0.0f, scale.y,
                0.0f, 0.0f,
            });
        }

        Float3x2 Float3x2::setRotation(float radians)
        {
            return Float3x2(
            {
                std::cos(radians), -std::sin(radians),
                std::sin(radians),  std::cos(radians),
            });
        }

        Float2 Float3x2::getScaling(void) const
        {
            return Float2(_11, _22);
        }

        Float2 Float3x2::operator [] (int row) const
        {
            return rows[row];
        }

        Float2 &Float3x2::operator [] (int row)
        {
            return rows[row];
        }

        Float3x2::operator const float *() const
        {
            return data;
        }

        Float3x2::operator float *()
        {
            return data;
        }

        void Float3x2::operator *= (const Float3x2 &matrix)
        {
            (*this) = ((*this) * matrix);
        }

        Float3x2 Float3x2::operator * (const Float3x2 &matrix) const
        {
            return Float3x2({ _11 * matrix._11 + _12 * matrix._21,
                _11 * matrix._12 + _12 * matrix._22,
                _21 * matrix._11 + _22 * matrix._21,
                _21 * matrix._12 + _22 * matrix._22,
                _31 * matrix._11 + _32 * matrix._21 + matrix._31,
                _31 * matrix._12 + _32 * matrix._22 + matrix._32 });
        }

        Float3x2 Float3x2::operator = (const Float3x2 &matrix)
        {
            rows[0] = matrix.rows[0];
            rows[1] = matrix.rows[1];
            rows[2] = matrix.rows[2];
            return (*this);
        }
    }; // namespace Math
}; // namespace Gek

#pragma once

#include <algorithm>
#include "GEK\Math\Common.h"
#include "GEK\Math\Vector2.h"


namespace Gek
{
    namespace Math
    {
        struct Float3x2
        {
        public:
            union
            {
                struct { float data[6]; };
                struct { float table[3][2]; };
                struct { Float2 rows[3]; };

                struct
                {
                    float _11, _12;
                    float _21, _22;
                    float _31, _32;
                };

                struct
                {
                    Float2 rx;
                    Float2 ry;
                    Float2 translation;
                };
            };

        public:
            Float3x2(void)
            {
                setIdentity();
            }

            Float3x2(const float (&data)[6])
                : data{ data[0], data[1], data[2], data[3], data[4], data[5] }
            {
            }

            Float3x2(const float *data)
                : data{ data[0], data[1], data[2], data[3], data[4], data[5] }
            {
            }

            Float3x2(const Float3x2 &matrix)
                : rows{ matrix.rows[0], matrix.rows[1], matrix.rows[2] }
            {
            }

            void setZero(void)
            {
                rows[0].set(0.0f, 0.0f);
                rows[1].set(0.0f, 0.0f);
                rows[2].set(0.0f, 0.0f);
            }

            void setIdentity(void)
            {
                rows[0].set(1.0f, 0.0f);
                rows[1].set(0.0f, 1.0f);
                rows[2].set(0.0f, 0.0f);
            }

            void setScaling(float scalar)
            {
                _11 = scalar;
                _22 = scalar;
            }

            void setScaling(const Float2 &vector)
            {
                _11 = vector.x;
                _22 = vector.y;
            }

            void setRotation(float radians)
            {
                rows[0].set(std::cos(radians), -std::sin(radians));
                rows[1].set(std::sin(radians), std::cos(radians));
                rows[2].set(0.0f, 0.0f);
            }

            Float2 getScaling(void) const
            {
                return Float2(_11, _22);
            }

            Float2 operator [] (int row) const
            {
                return rows[row];
            }

            Float2 &operator [] (int row)
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

            void operator *= (const Float3x2 &matrix)
            {
                (*this) = ((*this) * matrix);
            }

            Float3x2 operator * (const Float3x2 &matrix) const
            {
                return Float3x2({ _11 * matrix._11 + _12 * matrix._21,
                                  _11 * matrix._12 + _12 * matrix._22,
                                  _21 * matrix._11 + _22 * matrix._21,
                                  _21 * matrix._12 + _22 * matrix._22,
                                  _31 * matrix._11 + _32 * matrix._21 + matrix._31,
                                  _31 * matrix._12 + _32 * matrix._22 + matrix._32 });
            }

            Float3x2 operator = (const Float3x2 &matrix)
            {
                rows[0] = matrix.rows[0];
                rows[1] = matrix.rows[1];
                rows[2] = matrix.rows[2];
                return (*this);
            }
        };
    }; // namespace Math
}; // namespace Gek

/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Math/Vector2.hpp"

namespace Gek
{
    namespace Math
    {
        struct Float3x2
        {
        public:
            static const Float3x2 Identity;

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
                    union
                    {
                        struct { Float2 rz; };
                        struct { Float2 translation; };
                    };
                };
            };

        public:
            static Float3x2 FromScale(float scale)
            {
                return Float3x2(
                {
                    scale, 0.0f,
                    0.0f, scale,
                    0.0f, 0.0f,
                });
            }

            static Float3x2 FromScale(const Float2 &scale)
            {
                return Float3x2(
                {
                    scale.x, 0.0f,
                    0.0f, scale.y,
                    0.0f, 0.0f,
                });
            }

            static Float3x2 FromAngle(float radians)
            {
                return Float3x2(
                {
                    std::cos(radians), -std::sin(radians),
                    std::sin(radians),  std::cos(radians),
                    0.0f, 0.0f,
                });
            }

        public:
            inline Float3x2(void)
            {
            }

            inline Float3x2(const Float3x2 &matrix)
                : rows{
                matrix.rows[0],
                matrix.rows[1],
                matrix.rows[2] }
            {
            }

            inline Float3x2(float _11, float _12, float _21, float _22, float _31, float _32)
                : data{
                    _11, _12,
                    _21, _22,
                    _31, _32 }
            {
            }

            inline Float3x2(const float *data)
                : data {
                data[0], data[1],
                data[2], data[3],
                data[4], data[5] }
            {
            }

            inline Float2 getScaling(void) const
            {
                return Float2(_11, _22);
            }

            inline void operator *= (const Float3x2 &matrix)
            {
                (*this) = ((*this) * matrix);
            }

            inline Float3x2 operator * (const Float3x2 &matrix) const
            {
                return Float3x2({ _11 * matrix._11 + _12 * matrix._21,
                    _11 * matrix._12 + _12 * matrix._22,
                    _21 * matrix._11 + _22 * matrix._21,
                    _21 * matrix._12 + _22 * matrix._22,
                    _31 * matrix._11 + _32 * matrix._21 + matrix._31,
                    _31 * matrix._12 + _32 * matrix._22 + matrix._32 });
            }

            inline std::tuple<Float2, Float2, Float2> getTuple(void) const
            {
                return std::make_tuple(rx, ry, rz);
            }

            inline bool operator == (Float3x2 const &matrix) const
            {
                return (getTuple() == matrix.getTuple());
            }

            inline bool operator != (Float3x2 const &matrix) const
            {
                return (getTuple() != matrix.getTuple());
            }

            inline Float3x2 &operator = (Float3x2 const &matrix)
            {
                std::tie(rx, ry, rz) = matrix.getTuple();
                return (*this);
            }
        };
    }; // namespace Math
}; // namespace Gek

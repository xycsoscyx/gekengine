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
            static Float3x2 MakeScaling(Float2 const &scale, Float2 const &translation = Float2::Zero) noexcept
            {
                return Float3x2(
                {
                    scale.x, 0.0f,
                    0.0f, scale.y,
                    translation.x, translation.y,
                });
            }

            static Float3x2 MakeAngularRotation(float radians, Float2 const &translation = Float2::Zero) noexcept
            {
                return Float3x2(
                {
                    std::cos(radians), -std::sin(radians),
                    std::sin(radians),  std::cos(radians),
                    translation.x, translation.y,
                });
            }

        public:
            inline Float3x2(void) noexcept
            {
            }

            inline Float3x2(Float3x2 const &matrix) noexcept
                : rows{
                matrix.rows[0],
                matrix.rows[1],
                matrix.rows[2] }
            {
            }

            inline Float3x2(float _11, float _12, float _21, float _22, float _31, float _32) noexcept
                : data{
                    _11, _12,
                    _21, _22,
                    _31, _32 }
            {
            }

            inline Float3x2(const float *data) noexcept
                : data {
                data[0], data[1],
                data[2], data[3],
                data[4], data[5] }
            {
            }

            inline Float2 getScaling(void) const noexcept
            {
                return Float2(rx.getLength(), ry.getLength());
            }

            inline void operator *= (Float3x2 const &matrix) noexcept
            {
                (*this) = ((*this) * matrix);
            }

            inline Float3x2 operator * (Float3x2 const &matrix) const noexcept
            {
                return Float3x2({ _11 * matrix._11 + _12 * matrix._21,
                    _11 * matrix._12 + _12 * matrix._22,
                    _21 * matrix._11 + _22 * matrix._21,
                    _21 * matrix._12 + _22 * matrix._22,
                    _31 * matrix._11 + _32 * matrix._21 + matrix._31,
                    _31 * matrix._12 + _32 * matrix._22 + matrix._32 });
            }

            inline std::tuple<Float2, Float2, Float2> getTuple(void) const noexcept
            {
                return std::make_tuple(rx, ry, rz);
            }

            inline bool operator == (Float3x2 const &matrix) const noexcept
            {
                return (getTuple() == matrix.getTuple());
            }

            inline bool operator != (Float3x2 const &matrix) const noexcept
            {
                return (getTuple() != matrix.getTuple());
            }

            inline Float3x2 &operator = (Float3x2 const &matrix) noexcept
            {
                std::tie(rx, ry, rz) = matrix.getTuple();
                return (*this);
            }
        };
    }; // namespace Math
}; // namespace Gek

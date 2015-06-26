#pragma once

#include <cmath>
#include <algorithm>
#include <initializer_list>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector2;

        template <typename TYPE>
        struct BaseMatrix3x2
        {
        public:
            union
            {
                struct { TYPE data[6]; };
                struct { TYPE table[3][2]; };

                struct
                {
                    TYPE _11, _12;
                    TYPE _21, _22;
                    TYPE _31, _32;
                };

                struct
                {
                    BaseVector2<TYPE> rx;
                    BaseVector2<TYPE> ry;
                    BaseVector2<TYPE> translation;
                };
            };

        public:
            BaseMatrix3x2(void)
            {
                setIdentity();
            }

            BaseMatrix3x2(const std::initializer_list<float> &list)
            {
                std::copy(list.begin(), list.end(), data);
            }

            BaseMatrix3x2(const TYPE *vector)
            {
                std::copy_n(vector, 6, data);
            }

            BaseMatrix3x2(const BaseMatrix3x2<TYPE> &matrix)
            {
                std::copy_n(matrix.data, 6, data);
            }

            void setZero(void)
            {
                _11 = _12 = 0.0f;
                _21 = _22 = 0.0f;
                _31 = _32 = 0.0f;
            }

            void setIdentity(void)
            {
                _11 = _22 = 1.0f;
                _12 = 0.0f;
                _21 = 0.0f;
                _31 = _32 = 0.0f;
            }

            void setScaling(TYPE scalar)
            {
                _11 = scalar;
                _22 = scalar;
            }

            void setScaling(const BaseVector2<TYPE> &vector)
            {
                _11 = vector.x;
                _22 = vector.y;
            }

            void setRotation(float radians)
            {
                table[0][0] = std::cos(radians);
                table[1][0] = std::sin(radians);
                table[2][0] = 0.0f;

                table[0][1] = -std::sin(radians);
                table[1][1] = std::cos(radians);
                table[2][1] = 0.0f;
            }

            BaseVector2<TYPE> getScaling(void) const
            {
                return BaseVector2<TYPE>(_11, _22);
            }

            void operator *= (const BaseMatrix3x2<TYPE> &matrix)
            {
                (*this) = ((*this) * matrix);
            }

            BaseMatrix3x2<TYPE> operator * (const BaseMatrix3x2<TYPE> &matrix) const
            {
                return BaseMatrix3x2<TYPE>(_11 * matrix._11 + _12 * matrix._21,
                                           _11 * matrix._12 + _12 * matrix._22,
                                           _21 * matrix._11 + _22 * matrix._21,
                                           _21 * matrix._12 + _22 * matrix._22,
                                           _31 * matrix._11 + _32 * matrix._21 + matrix._31,
                                           _31 * matrix._12 + _32 * matrix._22 + matrix._32);
            }

            BaseMatrix3x2<TYPE> operator = (const BaseMatrix3x2<TYPE> &matrix)
            {
                memcpy(data, matrix.data, sizeof(data));
                return (*this);
            }
        };

        typedef BaseMatrix3x2<float> Float3x2;
    }; // namespace Math
}; // namespace Gek

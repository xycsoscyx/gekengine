/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c3a8e283af87669e3a3132e64063263f4eb7d446 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 15:54:27 2016 +0000 $
#pragma once

#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4SIMD.hpp"
#include <xmmintrin.h>
#include <type_traits>

namespace Gek
{
    namespace Math
    {
        namespace SIMD
        {
            template <typename TYPE, typename = typename std::enable_if<std::is_floating_point<TYPE>::value, TYPE>::type>
            struct Matrix4x4
            {
            public:
                static const Matrix4x4 Identity;

            public:
                union
                {
                    struct { TYPE data[16]; };
                    struct { TYPE table[4][4]; };
                    struct { Vector4<TYPE> rows[4]; };
                    struct { __m128 simd[4]; };

                    struct
                    {
                        TYPE _11, _12, _13, _14;
                        TYPE _21, _22, _23, _24;
                        TYPE _31, _32, _33, _34;
                        TYPE _41, _42, _43, _44;
                    };

                    struct
                    {
                        Vector4<TYPE> rx;
                        Vector4<TYPE> ry;
                        Vector4<TYPE> rz;
                        union
                        {
                            struct
                            {
                                Vector4<TYPE> rw;
                            };

                            struct
                            {
                                Vector3<TYPE> translation;
                                TYPE tw;
                            };
                        };
                    };

                    struct
                    {
                        struct
                        {
                            Vector3<TYPE> nx;
                            TYPE nxw;
                        };

                        struct
                        {
                            Vector3<TYPE> ny;
                            TYPE nyw;
                        };

                        struct
                        {
                            Vector3<TYPE> nz;
                            TYPE nzw;
                        };

                        struct
                        {
                            Vector3<TYPE> translation;
                            TYPE tw;
                        };
                    };

                    struct
                    {
                        struct
                        {
                            struct
                            {
                                Vector3<TYPE> n;
                                TYPE w;
                            } normals[3];
                        };

                        struct
                        {
                            Vector3<TYPE> translation;
                            TYPE tw;
                        };
                    };
                };

            public:
                Matrix4x4(void)
                {
                }

                Matrix4x4(
                    TYPE _11, TYPE _12, TYPE _13, TYPE _14,
                    TYPE _21, TYPE _22, TYPE _23, TYPE _24,
                    TYPE _31, TYPE _32, TYPE _33, TYPE _34,
                    TYPE _41, TYPE _42, TYPE _43, TYPE _44)
                    : simd{
                        _mm_setr_ps(_11, _12, _13, _14),
                        _mm_setr_ps(_21, _22, _23, _24),
                        _mm_setr_ps(_31, _32, _33, _34),
                        _mm_setr_ps(_41, _42, _43, _44) }
                {
                }

                Matrix4x4(const TYPE *data)
                    : simd{
                        _mm_loadu_ps(data + 0),
                        _mm_loadu_ps(data + 4),
                        _mm_loadu_ps(data + 8),
                        _mm_loadu_ps(data + 12) }
                {
                }

                Matrix4x4(
                    const __m128 &rx, 
                    const __m128 &ry,
                    const __m128 &rz,
                    const __m128 &rw)
                    : simd{ rx, ry, rz, rw }
                {
                }

                Matrix4x4(const Matrix4x4 &matrix)
                    : simd{ matrix.simd[0], matrix.simd[1], matrix.simd[2], matrix.simd[3] }
                {
                }

                Vector3<TYPE> getScaling(void) const
                {
                    return Vector3<TYPE>(_11, _22, _33);
                }

                TYPE getDeterminant(void) const
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

                Matrix4x4 getTranspose(void) const
                {
                    __m128 tmp3, tmp2, tmp1, tmp0;
                    tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
                    tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
                    tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
                    tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
                    return Matrix4x4(
                        _mm_shuffle_ps(tmp0, tmp1, 0x88),
                        _mm_shuffle_ps(tmp0, tmp1, 0xDD),
                        _mm_shuffle_ps(tmp2, tmp3, 0x88),
                        _mm_shuffle_ps(tmp2, tmp3, 0xDD));
                }

                Matrix4x4 getInverse(void) const
                {
                    TYPE determinant(getDeterminant());
                    if (std::abs(determinant) < Epsilon)
                    {
                        return Identity;
                    }
                    else
                    {
                        determinant = (1.0f / determinant);

                        return Matrix4x4(
                            (determinant * (table[1][1] * (table[2][2] * table[3][3] - table[3][2] * table[2][3]) + table[2][1] * (table[3][2] * table[1][3] - table[1][2] * table[3][3]) + table[3][1] * (table[1][2] * table[2][3] - table[2][2] * table[1][3]))),
                            (determinant * (table[1][2] * (table[2][0] * table[3][3] - table[3][0] * table[2][3]) + table[2][2] * (table[3][0] * table[1][3] - table[1][0] * table[3][3]) + table[3][2] * (table[1][0] * table[2][3] - table[2][0] * table[1][3]))),
                            (determinant * (table[1][3] * (table[2][0] * table[3][1] - table[3][0] * table[2][1]) + table[2][3] * (table[3][0] * table[1][1] - table[1][0] * table[3][1]) + table[3][3] * (table[1][0] * table[2][1] - table[2][0] * table[1][1]))),
                            (determinant * (table[1][0] * (table[3][1] * table[2][2] - table[2][1] * table[3][2]) + table[2][0] * (table[1][1] * table[3][2] - table[3][1] * table[1][2]) + table[3][0] * (table[2][1] * table[1][2] - table[1][1] * table[2][2]))),
                            (determinant * (table[2][1] * (table[0][2] * table[3][3] - table[3][2] * table[0][3]) + table[3][1] * (table[2][2] * table[0][3] - table[0][2] * table[2][3]) + table[0][1] * (table[3][2] * table[2][3] - table[2][2] * table[3][3]))),
                            (determinant * (table[2][2] * (table[0][0] * table[3][3] - table[3][0] * table[0][3]) + table[3][2] * (table[2][0] * table[0][3] - table[0][0] * table[2][3]) + table[0][2] * (table[3][0] * table[2][3] - table[2][0] * table[3][3]))),
                            (determinant * (table[2][3] * (table[0][0] * table[3][1] - table[3][0] * table[0][1]) + table[3][3] * (table[2][0] * table[0][1] - table[0][0] * table[2][1]) + table[0][3] * (table[3][0] * table[2][1] - table[2][0] * table[3][1]))),
                            (determinant * (table[2][0] * (table[3][1] * table[0][2] - table[0][1] * table[3][2]) + table[3][0] * (table[0][1] * table[2][2] - table[2][1] * table[0][2]) + table[0][0] * (table[2][1] * table[3][2] - table[3][1] * table[2][2]))),
                            (determinant * (table[3][1] * (table[0][2] * table[1][3] - table[1][2] * table[0][3]) + table[0][1] * (table[1][2] * table[3][3] - table[3][2] * table[1][3]) + table[1][1] * (table[3][2] * table[0][3] - table[0][2] * table[3][3]))),
                            (determinant * (table[3][2] * (table[0][0] * table[1][3] - table[1][0] * table[0][3]) + table[0][2] * (table[1][0] * table[3][3] - table[3][0] * table[1][3]) + table[1][2] * (table[3][0] * table[0][3] - table[0][0] * table[3][3]))),
                            (determinant * (table[3][3] * (table[0][0] * table[1][1] - table[1][0] * table[0][1]) + table[0][3] * (table[1][0] * table[3][1] - table[3][0] * table[1][1]) + table[1][3] * (table[3][0] * table[0][1] - table[0][0] * table[3][1]))),
                            (determinant * (table[3][0] * (table[1][1] * table[0][2] - table[0][1] * table[1][2]) + table[0][0] * (table[3][1] * table[1][2] - table[1][1] * table[3][2]) + table[1][0] * (table[0][1] * table[3][2] - table[3][1] * table[0][2]))),
                            (determinant * (table[0][1] * (table[2][2] * table[1][3] - table[1][2] * table[2][3]) + table[1][1] * (table[0][2] * table[2][3] - table[2][2] * table[0][3]) + table[2][1] * (table[1][2] * table[0][3] - table[0][2] * table[1][3]))),
                            (determinant * (table[0][2] * (table[2][0] * table[1][3] - table[1][0] * table[2][3]) + table[1][2] * (table[0][0] * table[2][3] - table[2][0] * table[0][3]) + table[2][2] * (table[1][0] * table[0][3] - table[0][0] * table[1][3]))),
                            (determinant * (table[0][3] * (table[2][0] * table[1][1] - table[1][0] * table[2][1]) + table[1][3] * (table[0][0] * table[2][1] - table[2][0] * table[0][1]) + table[2][3] * (table[1][0] * table[0][1] - table[0][0] * table[1][1]))),
                            (determinant * (table[0][0] * (table[1][1] * table[2][2] - table[2][1] * table[1][2]) + table[1][0] * (table[2][1] * table[0][2] - table[0][1] * table[2][2]) + table[2][0] * (table[0][1] * table[1][2] - table[1][1] * table[0][2]))));
                    }
                }

                Matrix4x4 getRotation(void) const
                {
                    // Sets row/column 4 to identity
                    return Matrix4x4(
                        _11, _12, _13, 0.0f,
                        _21, _22, _32, 0.0f,
                        _31, _32, _33, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);
                }

                Matrix4x4 &transpose(void)
                {
                    __m128 tmp3, tmp2, tmp1, tmp0;
                    tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
                    tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
                    tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
                    tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
                    simd[0] = _mm_shuffle_ps(tmp0, tmp1, 0x88);
                    simd[1] = _mm_shuffle_ps(tmp0, tmp1, 0xDD);
                    simd[2] = _mm_shuffle_ps(tmp2, tmp3, 0x88);
                    simd[3] = _mm_shuffle_ps(tmp2, tmp3, 0xDD);
                    return (*this);
                }

                Matrix4x4 &invert(void)
                {
                    (*this) = getInverse();
                    return (*this);
                }

                Vector4<TYPE> operator [] (int row) const
                {
                    return rows[row];
                }

                Vector4<TYPE> &operator [] (int row)
                {
                    return rows[row];
                }

                operator const TYPE *() const
                {
                    return data;
                }

                operator TYPE *()
                {
                    return data;
                }

                Matrix4x4 &operator = (const Matrix4x4 &matrix)
                {
                    rows[0] = matrix.rows[0];
                    rows[1] = matrix.rows[1];
                    rows[2] = matrix.rows[2];
                    rows[3] = matrix.rows[3];
                    return (*this);
                }

                void operator *= (const Matrix4x4 &matrix)
                {
                    simd[0] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                            _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                    simd[1] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                            _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                    simd[2] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                            _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                    simd[3] = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
                        _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
                        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
                            _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3])));
                }

                Matrix4x4 operator * (const Matrix4x4 &matrix) const
                {
                    return Matrix4x4(
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
                        _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])), _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]), _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))));
                }

                Vector3<TYPE> rotate(const Vector3<TYPE> &vector) const
                {
                    return Vector3<TYPE>(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)));
                }

                Vector3<TYPE> transform(const Vector3<TYPE> &vector) const
                {
                    return Vector3<TYPE>(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + _41),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + _42),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + _43));
                }

                Vector4<TYPE> transform(const Vector4<TYPE> &vector) const
                {
                    return Vector4<TYPE>(
                        ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                        ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                        ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                        ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)));
                }
            };

            using Float4x4 = Matrix4x4<float>;
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

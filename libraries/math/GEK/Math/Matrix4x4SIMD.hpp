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

				Matrix4x4(const __m128(&data)[4])
					: simd{ data[0], data[1], data[2], data[3] }
				{
				}

				Matrix4x4(const TYPE(&data)[16])
					: simd{ _mm_loadu_ps(data + 0),
					_mm_loadu_ps(data + 4),
					_mm_loadu_ps(data + 8),
					_mm_loadu_ps(data + 12) }
				{
				}

				Matrix4x4(const TYPE *data)
					: simd{ _mm_loadu_ps(data + 0),
					_mm_loadu_ps(data + 4),
					_mm_loadu_ps(data + 8),
					_mm_loadu_ps(data + 12) }
				{
				}

				Matrix4x4(const Matrix4x4 &matrix)
					: simd{ matrix.simd[0], matrix.simd[1], matrix.simd[2], matrix.simd[3] }
				{
				}

				static Matrix4x4 createScaling(TYPE scale)
				{
					return Matrix4x4(
					{
						scale, 0.0f, 0.0f, 0.0f,
						0.0f, scale, 0.0f, 0.0f,
						0.0f, 0.0f, scale, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createScaling(const Vector3<TYPE> &scale)
				{
					return Matrix4x4(
					{
						scale.x, 0.0f, 0.0f, 0.0f,
						0.0f, scale.y, 0.0f, 0.0f,
						0.0f, 0.0f, scale.z, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createEulerRotation(TYPE pitch, TYPE yaw, TYPE roll)
				{
					TYPE cosPitch(std::cos(pitch));
					TYPE sinPitch(std::sin(pitch));
					TYPE cosYaw(std::cos(yaw));
					TYPE sinYaw(std::sin(yaw));
					TYPE cosRoll(std::cos(roll));
					TYPE sinRoll(std::sin(roll));
					TYPE cosPitchSinYaw(cosPitch * sinYaw);
					TYPE sinPitchSinYaw(sinPitch * sinYaw);

					return Matrix4x4(
					{
						(cosYaw * cosRoll), (-cosYaw * sinRoll), sinYaw, 0.0f,
						(sinPitchSinYaw * cosRoll + cosPitch * sinRoll), (-sinPitchSinYaw * sinRoll + cosPitch * cosRoll), (-sinPitch * cosYaw), 0.0f,
						(-cosPitchSinYaw * cosRoll + sinPitch * sinRoll), (cosPitchSinYaw * sinRoll + sinPitch * cosRoll), (cosPitch * cosYaw), 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createAngularRotation(const Vector3<TYPE> &axis, TYPE radians)
				{
					TYPE cosAngle(std::cos(radians));
					TYPE sinAngle(std::sin(radians));

					return Matrix4x4(
					{
						(cosAngle + axis.x * axis.x * (1.0f - cosAngle)), (axis.z * sinAngle + axis.y * axis.x * (1.0f - cosAngle)), (-axis.y * sinAngle + axis.z * axis.x * (1.0f - cosAngle)), 0.0f,
						(-axis.z * sinAngle + axis.x * axis.y * (1.0f - cosAngle)), (cosAngle + axis.y * axis.y * (1.0f - cosAngle)), (axis.x * sinAngle + axis.z * axis.y * (1.0f - cosAngle)), 0.0f,
						(axis.y * sinAngle + axis.x * axis.z * (1.0f - cosAngle)), (-axis.x * sinAngle + axis.y * axis.z * (1.0f - cosAngle)), (cosAngle + axis.z * axis.z * (1.0f - cosAngle)), 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createPitchRotation(TYPE radians)
				{
					TYPE cosAngle(std::cos(radians));
					TYPE sinAngle(std::sin(radians));

					return Matrix4x4(
					{
						1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, cosAngle, sinAngle, 0.0f,
						0.0f, -sinAngle, cosAngle, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createYawRotation(TYPE radians)
				{
					TYPE cosAngle(std::cos(radians));
					TYPE sinAngle(std::sin(radians));

					return Matrix4x4(
					{
						cosAngle, 0.0f, -sinAngle, 0.0f,
						0.0f, 1.0f, 0.0f, 0.0f,
						sinAngle, 0.0f, cosAngle, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				static Matrix4x4 createRollRotation(TYPE radians)
				{
					TYPE cosAngle(std::cos(radians));
					TYPE sinAngle(std::sin(radians));

					return Matrix4x4(
					{
						cosAngle, sinAngle, 0.0f, 0.0f,
						-sinAngle, cosAngle, 0.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
				}

				// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
				static Matrix4x4 createOrthographic(TYPE left, TYPE top, TYPE right, TYPE bottom, TYPE nearClip, TYPE farClip)
				{
					return Matrix4x4(
					{
						(2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
						0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
						0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f,
						((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f,
					});
				}

				// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
				static Matrix4x4 createPerspective(TYPE fieldOfView, TYPE aspectRatio, TYPE nearClip, TYPE farClip)
				{
					TYPE yScale(1.0f / std::tan(fieldOfView * 0.5f));
					TYPE xScale(yScale / aspectRatio);
					TYPE denominator(farClip - nearClip);

					return Matrix4x4(
					{
						xScale, 0.0f, 0.0f, 0.0f,
						0.0f, yScale, 0.0f, 0.0f,
						0.0f, 0.0f, (farClip / denominator), 1.0f,
						0.0f, 0.0f, ((-nearClip * farClip) / denominator), 0.0f,
					});
				}

				static Matrix4x4 createLookAt(const Vector3<TYPE> &source, const Vector3<TYPE> &target, const Vector3<TYPE> &worldUpVector)
				{
					Vector3<TYPE> forward((target - source).getNormal());
					Vector3<TYPE> left(worldUpVector.cross(forward).getNormal());
					Vector3<TYPE> up(forward.cross(left));
					return Matrix4x4(
					{
						left.x,    left.y,    left.z, 0.0f,
						up.x,      up.y,      up.z, 0.0f,
						forward.x, forward.y, forward.z, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
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
						(table[0][2] * table[1][3] - table[1][2] * table[0][3])
						);
				}

				Matrix4x4 getTranspose(void) const
				{
					__m128 tmp3, tmp2, tmp1, tmp0;
					tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
					tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
					tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
					tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
					return Matrix4x4(
					{
						_mm_shuffle_ps(tmp0, tmp1, 0x88),
						_mm_shuffle_ps(tmp0, tmp1, 0xDD),
						_mm_shuffle_ps(tmp2, tmp3, 0x88),
						_mm_shuffle_ps(tmp2, tmp3, 0xDD),
					});
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

						Matrix4x4 matrix;
						matrix.table[0][0] = (determinant * (table[1][1] * (table[2][2] * table[3][3] - table[3][2] * table[2][3]) + table[2][1] * (table[3][2] * table[1][3] - table[1][2] * table[3][3]) + table[3][1] * (table[1][2] * table[2][3] - table[2][2] * table[1][3])));
						matrix.table[1][0] = (determinant * (table[1][2] * (table[2][0] * table[3][3] - table[3][0] * table[2][3]) + table[2][2] * (table[3][0] * table[1][3] - table[1][0] * table[3][3]) + table[3][2] * (table[1][0] * table[2][3] - table[2][0] * table[1][3])));
						matrix.table[2][0] = (determinant * (table[1][3] * (table[2][0] * table[3][1] - table[3][0] * table[2][1]) + table[2][3] * (table[3][0] * table[1][1] - table[1][0] * table[3][1]) + table[3][3] * (table[1][0] * table[2][1] - table[2][0] * table[1][1])));
						matrix.table[3][0] = (determinant * (table[1][0] * (table[3][1] * table[2][2] - table[2][1] * table[3][2]) + table[2][0] * (table[1][1] * table[3][2] - table[3][1] * table[1][2]) + table[3][0] * (table[2][1] * table[1][2] - table[1][1] * table[2][2])));
						matrix.table[0][1] = (determinant * (table[2][1] * (table[0][2] * table[3][3] - table[3][2] * table[0][3]) + table[3][1] * (table[2][2] * table[0][3] - table[0][2] * table[2][3]) + table[0][1] * (table[3][2] * table[2][3] - table[2][2] * table[3][3])));
						matrix.table[1][1] = (determinant * (table[2][2] * (table[0][0] * table[3][3] - table[3][0] * table[0][3]) + table[3][2] * (table[2][0] * table[0][3] - table[0][0] * table[2][3]) + table[0][2] * (table[3][0] * table[2][3] - table[2][0] * table[3][3])));
						matrix.table[2][1] = (determinant * (table[2][3] * (table[0][0] * table[3][1] - table[3][0] * table[0][1]) + table[3][3] * (table[2][0] * table[0][1] - table[0][0] * table[2][1]) + table[0][3] * (table[3][0] * table[2][1] - table[2][0] * table[3][1])));
						matrix.table[3][1] = (determinant * (table[2][0] * (table[3][1] * table[0][2] - table[0][1] * table[3][2]) + table[3][0] * (table[0][1] * table[2][2] - table[2][1] * table[0][2]) + table[0][0] * (table[2][1] * table[3][2] - table[3][1] * table[2][2])));
						matrix.table[0][2] = (determinant * (table[3][1] * (table[0][2] * table[1][3] - table[1][2] * table[0][3]) + table[0][1] * (table[1][2] * table[3][3] - table[3][2] * table[1][3]) + table[1][1] * (table[3][2] * table[0][3] - table[0][2] * table[3][3])));
						matrix.table[1][2] = (determinant * (table[3][2] * (table[0][0] * table[1][3] - table[1][0] * table[0][3]) + table[0][2] * (table[1][0] * table[3][3] - table[3][0] * table[1][3]) + table[1][2] * (table[3][0] * table[0][3] - table[0][0] * table[3][3])));
						matrix.table[2][2] = (determinant * (table[3][3] * (table[0][0] * table[1][1] - table[1][0] * table[0][1]) + table[0][3] * (table[1][0] * table[3][1] - table[3][0] * table[1][1]) + table[1][3] * (table[3][0] * table[0][1] - table[0][0] * table[3][1])));
						matrix.table[3][2] = (determinant * (table[3][0] * (table[1][1] * table[0][2] - table[0][1] * table[1][2]) + table[0][0] * (table[3][1] * table[1][2] - table[1][1] * table[3][2]) + table[1][0] * (table[0][1] * table[3][2] - table[3][1] * table[0][2])));
						matrix.table[0][3] = (determinant * (table[0][1] * (table[2][2] * table[1][3] - table[1][2] * table[2][3]) + table[1][1] * (table[0][2] * table[2][3] - table[2][2] * table[0][3]) + table[2][1] * (table[1][2] * table[0][3] - table[0][2] * table[1][3])));
						matrix.table[1][3] = (determinant * (table[0][2] * (table[2][0] * table[1][3] - table[1][0] * table[2][3]) + table[1][2] * (table[0][0] * table[2][3] - table[2][0] * table[0][3]) + table[2][2] * (table[1][0] * table[0][3] - table[0][0] * table[1][3])));
						matrix.table[2][3] = (determinant * (table[0][3] * (table[2][0] * table[1][1] - table[1][0] * table[2][1]) + table[1][3] * (table[0][0] * table[2][1] - table[2][0] * table[0][1]) + table[2][3] * (table[1][0] * table[0][1] - table[0][0] * table[1][1])));
						matrix.table[3][3] = (determinant * (table[0][0] * (table[1][1] * table[2][2] - table[2][1] * table[1][2]) + table[1][0] * (table[2][1] * table[0][2] - table[0][1] * table[2][2]) + table[2][0] * (table[0][1] * table[1][2] - table[1][1] * table[0][2])));
						return matrix;
					}
				}

				Matrix4x4 getRotation(void) const
				{
					// Sets row/column 4 to identity
					return Matrix4x4(
					{
						_11, _12, _13, 0.0f,
						_21, _22, _32, 0.0f,
						_31, _32, _33, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
					});
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
					{
						_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
						_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
						_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
						_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
						_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
						_mm_mul_ps(_mm_shuffle_ps(simd[1], simd[1], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
						_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
						_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
						_mm_mul_ps(_mm_shuffle_ps(simd[2], simd[2], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
						_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
						_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(1, 1, 1, 1)), matrix.simd[1])),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(2, 2, 2, 2)), matrix.simd[2]),
						_mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))),
					});
				}

				Vector3<TYPE> rotate(const Vector3<TYPE> &vector) const
				{
					return Vector3<TYPE>(
					{
						((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
						((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
						((vector.x * _13) + (vector.y * _23) + (vector.z * _33)),
					});
				}

				Vector3<TYPE> transform(const Vector3<TYPE> &vector) const
				{
					return Vector3<TYPE>(
					{
						((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + _41),
						((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + _42),
						((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + _43),
					});
				}

				Vector4<TYPE> transform(const Vector4<TYPE> &vector) const
				{
					return Vector4<TYPE>(
					{
						((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
						((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
						((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
						((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)),
					});
				}
			};

			using Float4x4 = Matrix4x4<float>;
		}; // namespace SIMD
    }; // namespace Math
}; // namespace Gek

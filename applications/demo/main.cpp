#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\Interface.h"
#include "GEK\Engine\CoreInterface.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include <CommCtrl.h>
#include <xmmintrin.h>
#include <array>
#include "resource.h"

#define GEKINLINE __forceinline

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

namespace Gek
{
    namespace MathSIMD
    {
        namespace Types
        {
            typedef __m128 Vector;
            typedef std::array<Vector, 4> Matrix;
        };

        namespace Vector
        {
            /**
                Set all four components to a single value.
                @param[in] value The value to set the components to
                @return The vector with all components set to [value]
            */
            GEKINLINE Types::Vector set(float value)
            {
                return _mm_set_ps1(value);
            }

            /**
                Set all four components to four separate values.
                Sets the values in reverse so that the internal order matches XYZW ordering
                @param[in] x The first component value
                @param[in] y The second component value
                @param[in] z The third component value
                @param[in] w The fourth component value
                @return The vector with components set to [XYZW]
            */
            GEKINLINE Types::Vector set(float x, float y, float z, float w)
            {
                return _mm_setr_ps(x, y, z, w);
            }

            /**
                Loads an array of four floats into the components
                @param[in] data The data to be loaded into the vector
                @return The vector with components set to [data]
            */
            GEKINLINE Types::Vector set(const float(&data)[4])
            {
                return _mm_loadu_ps(data);
            }

            /**
                Adds two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting vector of (left + right)
            */
            GEKINLINE Types::Vector add(Types::Vector left, Types::Vector right)
            {
                return _mm_add_ps(left, right);
            }

            /**
                Subtracts two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting vector of (left - right)
            */
            GEKINLINE Types::Vector subtract(Types::Vector left, Types::Vector right)
            {
                return _mm_sub_ps(left, right);
            }

            /**
                Multiplies two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting vector of (left * right)
            */
            GEKINLINE Types::Vector multiply(Types::Vector left, Types::Vector right)
            {
                return _mm_mul_ps(left, right);
            }

            /**
                Divides two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting vector of (left / right)
            */
            GEKINLINE Types::Vector divide(Types::Vector left, Types::Vector right)
            {
                return _mm_div_ps(left, right);
            }

            /**
                Calculates the dot product of two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The result of (lx*rx)+(ly*ry)+(lz*rz)+(lw*rw) in all four components
            */
            GEKINLINE Types::Vector dot(Types::Vector left, Types::Vector right)
            {
                Types::Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
                Types::Vector YXZW = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(2, 3, 0, 1)); // ry, rx, lw, lz
                Types::Vector add = _mm_add_ps(multiply, YXZW);
                Types::Vector ZWYX = _mm_shuffle_ps(add, add, _MM_SHUFFLE(0, 1, 2, 3)); // rz, rw, ly, lx
                return _mm_add_ps(ZWYX, add);
            }

            /**
                Calculates the square root of each component in a vector
                @param[in] value The value to calculate the square root of
                @return The resulting vector of sqrt(value)
            */
            GEKINLINE Types::Vector sqrt(Types::Vector value)
            {
                return _mm_sqrt_ps(value);
            }

            /**
                Calculates the squared length of a vector
                @param[in] value The value to calculate the squared length of
                @return The squared length of the vector in all four components
            */
            GEKINLINE Types::Vector lengthSquared(Types::Vector vector)
            {
                return dot(vector, vector);
            }

            /**
                Calculates the length of a vector
                @param[in] value The value to calculate the length of
                @return The length of the vector in all four components
            */
            GEKINLINE Types::Vector length(Types::Vector vector)
            {
                return sqrt(lengthSquared(vector));
            }

            /**
                Calculates the normal vector of a (potentially) un-normalized vector
                @param[in] value The value to calculate the normal of
                @return The resulting normalized vector from (value / length(value))
            */
            GEKINLINE Types::Vector normal(Types::Vector value)
            {
                return divide(value, length(value));
            }

            /**
                Calculates the linear interpolation of two vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @param[in] factor The factor to interpolate by
                @return The resulting vector of (((right - left) * factor) + left)
            */
            GEKINLINE Types::Vector lerp(Types::Vector left, Types::Vector right, float factor)
            {
                return add(left, multiply(subtract(right, left), set(factor)));
            }

            GEKINLINE Types::Vector isEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpeq_ps(left, right);
            }

            GEKINLINE Types::Vector isNotEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpneq_ps(left, right);
            }

            GEKINLINE Types::Vector isLess(Types::Vector left, Types::Vector right)
            {
                return _mm_cmplt_ps(left, right);
            }

            GEKINLINE Types::Vector isLessEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmple_ps(left, right);
            }

            GEKINLINE Types::Vector isGreater(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpgt_ps(left, right);
            }

            GEKINLINE Types::Vector isGreaterEqual(Types::Vector left, Types::Vector right)
            {
                return _mm_cmpge_ps(left, right);
            }
        };

        namespace Vector3
        {
            /**
                Calculates the dot product of two vectors, ignoring the W component
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The result of (lx*rx)+(ly*ry)+(lz*rz) in all four components
            */
            GEKINLINE Types::Vector dot(Types::Vector left, Types::Vector right)
            {
                Types::Vector multiply = _mm_mul_ps(left, right); // lx*rx, ly*ry, lz*rz, lw*rw
                Types::Vector Y = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 1)); // get lyry in a
                Types::Vector Z = _mm_shuffle_ps(multiply, multiply, _MM_SHUFFLE(0, 0, 0, 2)); // get lzrz in a
                Types::Vector addYZ = _mm_add_ps(Y, Z); // lyry+lzrz
                Types::Vector addXYZ = _mm_add_ps(multiply, addYZ); // xx+(lyry+lzrz)
                return _mm_shuffle_ps(addXYZ, addXYZ, _MM_SHUFFLE(0, 0, 0, 0)); // shuffle (lxrx+lyry+lzrz) to all four
            }

            /**
                Calculates the cross product of two vectors, ignoring the W component
                @param[in] left
                @param[in] right
                @return The resulting vector of { ((ly*rz)-(lz*ry)), ((lz*rx)-(lx*rz)), ((lx*ry)-(ly*rx)), 0 }
            */
            GEKINLINE Types::Vector cross(Types::Vector left, Types::Vector right)
            {
                Types::Vector shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 0, 2, 1));
                Types::Vector shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 1, 0, 2));
                Types::Vector multiply0 = _mm_mul_ps(shuffle0, shuffle1); // (y1*z2), (z1*x2), (x1*y2), (x1*x2)
                shuffle0 = _mm_shuffle_ps(left, left, _MM_SHUFFLE(0, 1, 0, 2));
                shuffle1 = _mm_shuffle_ps(right, right, _MM_SHUFFLE(0, 0, 2, 1));
                Types::Vector multiply1 = _mm_mul_ps(shuffle0, shuffle1); // (z1*y2), (x1*z2), (y1*x2), (x1*x2)
                return _mm_sub_ps(multiply0, multiply1);
            }

            /**
                Calculates the squared length of a vector, ignoring the W component
                @param[in] value The value to calculate the squared length of
                @return The squared length of the vector in all four components
            */
            GEKINLINE Types::Vector lengthSquared(Types::Vector vector)
            {
                return dot(vector, vector);
            }

            /**
                Calculates the length of a vector, ignoring the W component
                @param[in] value The value to calculate the length of
                @return The length of the vector in all four components
            */
            GEKINLINE Types::Vector length(Types::Vector vector)
            {
                return Vector::sqrt(lengthSquared(vector));
            }

            /**
                Calculates the normal vector of a (potentially) un-normalized vector, ignoring the W component
                @param[in] value The value to calculate the normal of
                @return The resulting normalized vector from (value / length(value))
            */
            GEKINLINE Types::Vector normal(Types::Vector value)
            {
                return Vector::divide(value, length(value));
            }
        };

        namespace Quaternion
        {
            /**
                Converts the rotational part of a matrix into a quaternion vector
                @param[in] value The matrix to convert to a quaternion vector
                @return The resulting quaternion vector of the rotational part of the input matrix
            */
            GEKINLINE Types::Vector set(const Types::Matrix &value)
            {
            }

            /**
                Multiplies two quaternion vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting quaternion vector of (left * right)
            */
            GEKINLINE Types::Vector multiply(Types::Vector left, Types::Vector right)
            {
            }

            /**
                Calculates the spherical interpolation of two quaternion vectors
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @param[in] factor The factor to interpolate by
                @return The resulting quaternion vector of the interpolation operation
            */
            GEKINLINE Types::Vector slerp(Types::Vector left, Types::Vector right, float factor)
            {
            }

            /**
                Calculates the inverse of a quaternion vector
                @param[in] matrix The quaternion vector to invert
                @return The resulting inverse quaternion vector of the input
            */
            GEKINLINE Types::Vector invert(const Types::Vector &quaternion)
            {
            }
        };

        namespace Matrix
        {
            /**
                Loads an array of sixteen floats into the matrix
                @param[in] data The data to be loaded into the matrix
                @return The matrix with components set to [data]
            */
            GEKINLINE Types::Matrix set(const float(&data)[16])
            {
                return{ _mm_loadu_ps(data), _mm_loadu_ps(data + 4), _mm_loadu_ps(data + 8), _mm_loadu_ps(data + 12) };
            }

            /**
                Loads an array of four vectors into the matrix
                @param[in] data The data to be loaded into the matrix
                @return The matrix with components set to [data]
            */
            GEKINLINE Types::Matrix set(const Types::Vector(&data)[4])
            {
                return{ data[0], data[1], data[2], data[3] };
            }

            /**
                Converts Euler angles to a rotational matrix
                @param[in] pitch The pitch to set the rotation matrix to (x axis rotation)
                @param[in] roll The roll to set the rotation matrix to (y axis rotation)
                @param[in] yaw The yaw to set the rotation matrix to (z axis rotation)
                @return The resulting rotation matrix from the Euler angles
            */
            GEKINLINE Types::Matrix set(float pitch, float roll, float yaw)
            {
                float cosPitch(std::cos(pitch));
                float sinPitch(std::sin(pitch));
                float cosRoll(std::cos(roll));
                float sinRoll(std::sin(roll));
                float cosYaw(std::cos(yaw));
                float sinYaw(std::sin(yaw));
                float cosPitchSinRoll(cosPitch * sinRoll);
                float sinPitchSinRoll(sinPitch * sinRoll);

                return set({ ( cosRoll * cosYaw), ( sinPitchSinRoll * cosYaw + cosPitch * sinYaw), (-cosPitchSinRoll * cosYaw + sinPitch * sinYaw), 0.0f,
                             (-cosRoll * sinYaw), (-sinPitchSinRoll * sinYaw + cosPitch * cosYaw), ( cosPitchSinRoll * sinYaw + sinPitch * cosYaw), 0.0f,
                               sinRoll, (-sinPitch * cosRoll), (cosPitch * cosRoll), 0.0f,
                               0.0f, 0.0f, 0.0f, 1.0f, });
            }

            /**
                Converts a quaternion vector to a rotational matrix
                @param[in] quaternion The quaternion vector to convert to a rotational matrix
                @return The resulting matrix from the quaternion vector
            */
            GEKINLINE Types::Matrix set(Types::Vector quaternion)
            {
                float xy(quaternion.m128_f32[0] * quaternion.m128_f32[1]);
                float zw(quaternion.m128_f32[2] * quaternion.m128_f32[3]);
                float xz(quaternion.m128_f32[0] * quaternion.m128_f32[2]);
                float yw(quaternion.m128_f32[1] * quaternion.m128_f32[3]);
                float yz(quaternion.m128_f32[1] * quaternion.m128_f32[2]);
                float xw(quaternion.m128_f32[0] * quaternion.m128_f32[3]);
                Types::Vector square = Vector::multiply(quaternion, quaternion);
                float determinant(1.0f / (square.m128_f32[0] + square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]));

                return set({  ((square.m128_f32[0] - square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f,
                             (2.0f * (xy - zw) * determinant), ((-square.m128_f32[0] + square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (yz + xw) * determinant), 0.0f,
                             (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-square.m128_f32[0] - square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]) * determinant), 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f });
            }

            /**
                Converts a quaternion vector to a rotational matrix, setting the translation component of the matrix after conversion
                @param[in] quaternion The quaternion vector to convert to a rotational matrix
                @param[in] translation The translation component to set in the rotational matrix
                @return The resulting matrix from the quaternion vector and translation component
            */
            GEKINLINE Types::Matrix set(Types::Vector quaternion, Types::Vector translation)
            {
                float xy(quaternion.m128_f32[0] * quaternion.m128_f32[1]);
                float zw(quaternion.m128_f32[2] * quaternion.m128_f32[3]);
                float xz(quaternion.m128_f32[0] * quaternion.m128_f32[2]);
                float yw(quaternion.m128_f32[1] * quaternion.m128_f32[3]);
                float yz(quaternion.m128_f32[1] * quaternion.m128_f32[2]);
                float xw(quaternion.m128_f32[0] * quaternion.m128_f32[3]);
                Types::Vector square = Vector::multiply(quaternion, quaternion);
                float determinant(1.0f / (square.m128_f32[0] + square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]));

                return set({ Vector::set({  ((square.m128_f32[0] - square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f, }),
                             Vector::set({ (2.0f * (xy - zw) * determinant), ((-square.m128_f32[0] + square.m128_f32[1] - square.m128_f32[2] + square.m128_f32[3]) * determinant), (2.0f * (yz + xw) * determinant), 0.0f, }),
                             Vector::set({ (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-square.m128_f32[0] - square.m128_f32[1] + square.m128_f32[2] + square.m128_f32[3]) * determinant), 0.0f, }),
                             translation });
            }

            /**
                Creates a perspective projection matrix, scales far objects to zero
                @param[in] fieldOfView The horizontal field of view (in radians) of the perspective projection
                @param[in] aspectRatio The aspect ratio of the perspective projection (vertical / horizontal)
                @param[in] nearDepth The near clipping plane of the perspective projection
                @param[in] farDepth The far clipping plane of the perspective projection
                @return The resulting perspective projection matrix
            */
            GEKINLINE Types::Matrix set(float fieldOfView, float aspectRatio, float nearDepth, float farDepth)
            {
                float x(1.0f / std::tan(fieldOfView * 0.5f));
                float y(x * aspectRatio);
                float distance(farDepth - nearDepth);

                return set({ x, 0.0f, 0.0f, 0.0f,
                             0.0f, y, 0.0f, 0.0f,
                             0.0f, 0.0f, ((farDepth + nearDepth) / distance), 1.0f,
                             0.0f, 0.0f, -((2.0f * farDepth * nearDepth) / distance), 0.0f, });
            }

            /**
                Creates an orthographic projection matrix, no scale regardless of depth
                @param[in] left The left clipping plane of the orthographic priojection
                @param[in] top The top clipping plane of the orthographic priojection
                @param[in] right The right clipping plane of the orthographic priojection
                @param[in] bottom The bottom clipping plane of the orthographic priojection
                @param[in] nearDepth The near clipping plane of the orthographic projection
                @param[in] farDepth The far clipping plane of the orthographic projection
                @return The resulting orthographic projection matrix
            */
            GEKINLINE Types::Matrix set(float left, float top, float right, float bottom, float nearDepth, float farDepth)
            {
                return set({ (2.0f / (right - left)), 0.0f, 0.0f, 0.0f,
                              0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f,
                              0.0f, 0.0f, (-2.0f / (farDepth - nearDepth)), 0.0f,
                    -((right + left) / (right - left)), -((top + bottom) / (top - bottom)), -((farDepth + nearDepth) / (farDepth - nearDepth)), 1.0f, });
            }

            /**
                Calculates the inverse of a matrix
                @param[in] matrix The matrix to invert
                @return The resulting inverse matrix of the input
            */
            GEKINLINE Types::Matrix invert(const Types::Matrix &matrix)
            {
            }

            /**
                Calculates the transpose of a matrix
                @param[in] matrix The matrix to transpose
                @return The resulting transposed matrix of the input
            */
            GEKINLINE Types::Matrix transpose(const Types::Matrix &matrix)
            {
                Types::Matrix unpacked =
                {
                    _mm_unpacklo_ps(matrix[0], matrix[2]),
                    _mm_unpackhi_ps(matrix[0], matrix[2]),
                    _mm_unpacklo_ps(matrix[1], matrix[3]),
                    _mm_unpackhi_ps(matrix[1], matrix[3]),
                };

                return
                {
                    _mm_unpacklo_ps(unpacked[0], unpacked[2]),
                    _mm_unpackhi_ps(unpacked[0], unpacked[2]),
                    _mm_unpacklo_ps(unpacked[1], unpacked[3]),
                    _mm_unpackhi_ps(unpacked[1], unpacked[3]),
                };
            }

            /**
                Multiplies a vector by a matrix
                @param[in] matrix The matrix to multiply by
                @param[in] vetor The vector to multiply
                @return The resulting vector of (matrix * vector)
            */
            GEKINLINE Types::Vector multiply(const Types::Matrix &matrix, Types::Vector vector)
            {
                Types::Matrix shuffled =
                {
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(0, 0, 0, 0)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(1, 1, 1, 1)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(2, 2, 2, 2)),
                    _mm_shuffle_ps(vector, vector, _MM_SHUFFLE(3, 3, 3, 3)),
                };

                return _mm_add_ps(
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[0], matrix[0]),
                        _mm_mul_ps(shuffled[1], matrix[1])),
                    _mm_add_ps(
                        _mm_mul_ps(shuffled[2], matrix[2]),
                        _mm_mul_ps(shuffled[3], matrix[3])));
            }

            /**
                Multiplies two matrices
                @param[in] left The left component in the operation
                @param[in] right The right component in the operation
                @return The resulting matrix of (left * right)
            */
            GEKINLINE Types::Matrix multiply(const Types::Matrix &left, const Types::Matrix &right)
            {
                Types::Matrix result;
                for (UINT32 rowIndex = 0; rowIndex < 4; rowIndex++)
                {
                    Types::Matrix shuffled =
                    {
                        _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(0, 0, 0, 0)),
                        _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(1, 1, 1, 1)),
                        _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(2, 2, 2, 2)),
                        _mm_shuffle_ps(left[rowIndex], left[rowIndex], _MM_SHUFFLE(3, 3, 3, 3)),
                    };

                    result[rowIndex] = _mm_add_ps(
                        _mm_add_ps(
                            _mm_mul_ps(shuffled[0], right[0]),
                            _mm_mul_ps(shuffled[1], right[1])),
                        _mm_add_ps(
                            _mm_mul_ps(shuffled[2], right[2]),
                            _mm_mul_ps(shuffled[3], right[3])));
                }

                return result;
            }
        };

        void test(void)
        {
            BUILD_BUG_ON(sizeof(Types::Vector) != 16);
            BUILD_BUG_ON(sizeof(Types::Matrix) != 64);

            Types::Vector vector = Vector::set({ 1.0f, 2.0f, 4.0f, 4.0f });

            Types::Vector xAxis = Vector::set(1.0f, 0.0f, 0.0f, 0.0f);
            Types::Vector yAxis = Vector::set(0.0f, 1.0f, 0.0f, 0.0f);
            Types::Vector zAxis = Vector3::cross(xAxis, yAxis);

            Types::Vector vectorA = Vector::set(1.0f, 2.0f, 3.0f, 4.0f);
            Types::Vector vectorB = Vector::set(5.0f, 6.0f, 7.0f, 8.0f);
            Types::Vector cross3Result = Vector3::cross(vectorA, vectorB);
            Types::Vector normalA = Vector::normal(vectorA);
            Types::Vector normalA3 = Vector3::normal(vectorA);

            Types::Vector lengthResult = Vector::length(vectorA);
            Types::Vector length3Result = Vector3::length(vectorA);
            Types::Vector dotResult = Vector::dot(vectorA, vectorB);
            Types::Vector dot3Result = Vector3::dot(vectorA, vectorB);
            Types::Vector addResult = Vector::add(vectorA, vectorB);
            Types::Vector subtractResult = Vector::subtract(vectorA, vectorB);
            Types::Vector multiplyResult = Vector::multiply(vectorA, vectorB);
            Types::Vector divideResult = Vector::divide(vectorA, vectorB);

            static const float identityData[16] =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };

            Types::Matrix identity = Matrix::set(identityData);

            static const float scaleData[16] = 
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 2.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 3.0f, 0.0f,
                4.0f, 5.0f, 6.0f, 1.0f,
            };

            Types::Matrix scale = Matrix::set(scaleData);

            Types::Matrix multiplyResult1 = Matrix::multiply(identity, scale);
            Types::Matrix multiplyResult2 = Matrix::multiply(scale, identity);
            Types::Matrix multiplyResult3 = Matrix::multiply(scale, scale);
            
            static const float fullData[16] =
            {
                 1.0f,  2.0f,  3.0f,  4.0f,
                 5.0f,  6.0f,  7.0f,  8.0f,
                 9.0f, 10.0f, 11.0f, 12.0f,
                13.0f, 14.0f, 15.0f, 16.0f,
            };

            Types::Matrix full = Matrix::set(fullData);
            Types::Matrix transposeResult = Matrix::transpose(full);
            Types::Matrix multiplyResult4 = Matrix::multiply(full, transposeResult);
            
            Types::Vector base = Vector::set(1.0f, 1.0f, 1.0f, 1.0f);
            Types::Vector base3 = Vector::set(1.0f, 1.0f, 1.0f, 0.0f);
            Types::Vector multiplyBase = Matrix::multiply(scale, base);
            Types::Vector multiplyBase3 = Matrix::multiply(scale, base3);

            Types::Matrix eulerX = Matrix::setEuler(Gek::Math::convertDegreesToRadians(90.0f), 0.0f, 0.0f);
            Types::Matrix eulerY = Matrix::setEuler(0.0f, Gek::Math::convertDegreesToRadians(90.0f), 0.0f);
            Types::Matrix eulerZ = Matrix::setEuler(0.0f, 0.0f, Gek::Math::convertDegreesToRadians(90.0f));
            Types::Vector quaternion = Vector::set(0.0f, 0.0f, 0.0f, 1.0f);
            Types::Matrix matrix = Matrix::setQuaternion(quaternion);

            Types::Vector xxMultiplyResult = Matrix::multiply(eulerX, xAxis);
            Types::Vector xyMultiplyResult = Matrix::multiply(eulerX, yAxis);
            Types::Vector xzMultiplyResult = Matrix::multiply(eulerX, zAxis);
            Types::Vector yxMultiplyResult = Matrix::multiply(eulerY, xAxis);
            Types::Vector yyMultiplyResult = Matrix::multiply(eulerY, yAxis);
            Types::Vector yzMultiplyResult = Matrix::multiply(eulerY, zAxis);
            Types::Vector zxMultiplyResult = Matrix::multiply(eulerZ, xAxis);
            Types::Vector zyMultiplyResult = Matrix::multiply(eulerZ, yAxis);
            Types::Vector zzMultiplyResult = Matrix::multiply(eulerZ, zAxis);
        }
    };
};

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
        {
            UINT32 width = 800;
            UINT32 height = 600;
            bool windowed = true;

            Gek::Xml::Document xmlDocument;
            if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
            {
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                {
                    Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                    if (xmlDisplayNode)
                    {
                        if (xmlDisplayNode.hasAttribute(L"width"))
                        {
                            width = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"width"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"height"))
                        {
                            height = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"height"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"windowed"))
                        {
                            windowed = Gek::String::getBoolean(xmlDisplayNode.getAttribute(L"windowed"));
                        }
                    }
                }
            }

            UINT32 selectIndex = 0;
            SendDlgItemMessage(dialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
            std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
            for(auto &mode : modeList)
            {
                CStringW aspectRatio(L"");
                switch (mode.aspectRatio)
                {
                case Gek::Display::AspectRatio::_4x3:
                    aspectRatio = L", (4x3)";
                    break;

                case Gek::Display::AspectRatio::_16x9:
                    aspectRatio = L", (16x9)";
                    break;

                case Gek::Display::AspectRatio::_16x10:
                    aspectRatio = L", (16x10)";
                    break;
                };

                CStringW modeString;
                modeString.Format(L"%dx%d%s", mode.width, mode.height, aspectRatio.GetString());
                int modeIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)modeString.GetString());
                if (mode.width == width && mode.height == height)
                {
                    selectIndex = modeIndex;
                }
            }

            SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_SETCHECK, windowed ? BST_UNCHECKED : BST_CHECKED, 0);

            SendDlgItemMessage(dialog, IDC_MODES, CB_SETMINVISIBLE, 5, 0);
            SendDlgItemMessage(dialog, IDC_MODES, CB_SETEXTENDEDUI, TRUE, 0);
            SendDlgItemMessage(dialog, IDC_MODES, CB_SETCURSEL, selectIndex, 0);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
                UINT32 selectIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_GETCURSEL, 0, 0);
                auto &mode = modeList[selectIndex];

                Gek::Xml::Document xmlDocument;
                xmlDocument.load(L"%root%\\config.xml");
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (!xmlConfigNode || xmlConfigNode.getType().CompareNoCase(L"config") != 0)
                {
                    xmlDocument.create(L"config");
                    xmlConfigNode = xmlDocument.getRoot();
                }

                Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                if (!xmlDisplayNode)
                {
                    xmlDisplayNode = xmlConfigNode.createChildElement(L"display");
                }

                xmlDisplayNode.setAttribute(L"width", L"%d", mode.width);
                xmlDisplayNode.setAttribute(L"height", L"%d", mode.height);
                xmlDisplayNode.setAttribute(L"windowed", L"%s", (SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_UNCHECKED ? L"true" : L"false"));
                xmlDocument.save(L"%root%\\config.xml");

                EndDialog(dialog, IDOK);
                return TRUE;
            }

        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

LRESULT CALLBACK WindowProc(HWND window, UINT32 message, WPARAM wParam, LPARAM lParam)
{
    LRESULT resultValue = 0;
    Gek::Engine::Core::Interface *engineCore = (Gek::Engine::Core::Interface *)GetWindowLongPtr(window, GWLP_USERDATA);
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        if (engineCore)
        {
            resultValue = engineCore->windowEvent(message, wParam, lParam);
        }

        break;
    };

    if (resultValue == 0)
    {
        resultValue = DefWindowProc(window, message, wParam, lParam);
    }

    return resultValue;
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    Gek::MathSIMD::test();

    return 0;

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        CComPtr<Gek::Context::Interface> context;
        Gek::Context::create(&context);
        if (context)
        {
#ifdef _DEBUG
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
            context->addSearchPath(L"%root%\\Debug\\Plugins");
#else
            SetCurrentDirectory(GEKParseFileName(L"%root%\\Release"));
            context->AddSearchPath(GEKParseFileName(L"%root%\\Release\\Plugins"));
#endif

            context->initialize();
            CComPtr<Gek::Engine::Core::Interface> engineCore;
            context->createInstance(CLSID_IID_PPV_ARGS(Gek::Engine::Core::Class, &engineCore));
            if (engineCore)
            {
                WNDCLASS kClass;
                kClass.style = 0;
                kClass.lpfnWndProc = WindowProc;
                kClass.cbClsExtra = 0;
                kClass.cbWndExtra = 0;
                kClass.hInstance = GetModuleHandle(nullptr);
                kClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
                kClass.hCursor = nullptr;
                kClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                kClass.lpszMenuName = nullptr;
                kClass.lpszClassName = L"GEKvX_Engine_314159";
                if (RegisterClass(&kClass))
                {
                    UINT32 width = 800;
                    UINT32 height = 600;
                    bool windowed = true;

                    Gek::Xml::Document xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
                    {
                        Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                        if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                        {
                            Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                            if (xmlDisplayNode)
                            {
                                if (xmlDisplayNode.hasAttribute(L"width"))
                                {
                                    width = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"width"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"height"))
                                {
                                    height = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"height"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"windowed"))
                                {
                                    windowed = Gek::String::getBoolean(xmlDisplayNode.getAttribute(L"windowed"));
                                }
                            }
                        }
                    }

                    RECT clientRect;
                    clientRect.left = 0;
                    clientRect.top = 0;
                    clientRect.right = width;
                    clientRect.bottom = height;
                    AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);
                    int windowWidth = (clientRect.right - clientRect.left);
                    int windowHeight = (clientRect.bottom - clientRect.top);
                    int centerPositionX = (windowed ? (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((clientRect.right - clientRect.left) / 2) : 0);
                    int centerPositionY = (windowed ? (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((clientRect.bottom - clientRect.top) / 2) : 0);
                    HWND window = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, centerPositionX, centerPositionY, windowWidth, windowHeight, 0, nullptr, GetModuleHandle(nullptr), 0);
                    if (window)
                    {
                        if (SUCCEEDED(engineCore->initialize(window)))
                        {
                            SetWindowLongPtr(window, GWLP_USERDATA, LONG((Gek::Engine::Core::Interface *)engineCore));
                            ShowWindow(window, SW_SHOW);
                            UpdateWindow(window);

                            context->logMessage(__FILE__, __LINE__, L"[entering] Game Loop");
                            context->logEnterScope();

                            MSG message = { 0 };
                            while (message.message != WM_QUIT)
                            {
                                while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                                {
                                    TranslateMessage(&message);
                                    DispatchMessage(&message);
                                };

                                if (!engineCore->update())
                                {
                                    break;
                                }
                            };

                            context->logExitScope();
                            context->logMessage(__FILE__, __LINE__, L"[entering] Game Loop");

                            SetWindowLongPtr(window, GWLP_USERDATA, 0);
                            engineCore.Release();
                            DestroyWindow(window);
                        }
                    }
                    else
                    {
                        context->logMessage(__FILE__, __LINE__, L"Unable to create window: %d", GetLastError());
                    }
                }
                else
                {
                    context->logMessage(__FILE__, __LINE__, L"Unable to register window class: %d", GetLastError());
                }
            }
        }

        return 0;
    }

    return -1;
}
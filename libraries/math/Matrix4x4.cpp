#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Float4x4::Float4x4(void)
        {
            setIdentity();
        }

        Float4x4::Float4x4(const __m128(&data)[4])
            : simd{ data[0], data[1], data[2], data[3] }
        {
        }

        Float4x4::Float4x4(const float(&data)[16])
            : simd{ _mm_loadu_ps(data +  0),
                    _mm_loadu_ps(data +  4),
                    _mm_loadu_ps(data +  8),
                    _mm_loadu_ps(data + 12) }
        {
        }

        Float4x4::Float4x4(const float *data)
            : simd{ _mm_loadu_ps(data +  0),
                    _mm_loadu_ps(data +  4),
                    _mm_loadu_ps(data +  8),
                    _mm_loadu_ps(data + 12) }
        {
        }

        Float4x4::Float4x4(const Float4x4 &matrix)
            : simd{ matrix.simd[0], matrix.simd[1], matrix.simd[2], matrix.simd[3] }
        {
        }

        Float4x4::Float4x4(const Float3 &euler)
        {
            setEuler(euler.x, euler.y, euler.z);
        }

        Float4x4::Float4x4(float x, float y, float z)
        {
            setEuler(x, y, z);
        }

        Float4x4::Float4x4(const Float3 &axis, float radians)
        {
            setRotation(axis, radians);
        }

        Float4x4::Float4x4(const Quaternion &rotation)
        {
            setRotation(rotation);
        }

        Float4x4::Float4x4(const Quaternion &rotation, const Float3 &translation)
        {
            setRotation(rotation, translation);
        }

        void Float4x4::setZero(void)
        {
            rows[0].set(0.0f, 0.0f, 0.0f, 0.0f);
            rows[1].set(0.0f, 0.0f, 0.0f, 0.0f);
            rows[2].set(0.0f, 0.0f, 0.0f, 0.0f);
            rows[3].set(0.0f, 0.0f, 0.0f, 0.0f);
        }

        void Float4x4::setIdentity(void)
        {
            rows[0].set(1.0f, 0.0f, 0.0f, 0.0f);
            rows[1].set(0.0f, 1.0f, 0.0f, 0.0f);
            rows[2].set(0.0f, 0.0f, 1.0f, 0.0f);
            rows[3].set(0.0f, 0.0f, 0.0f, 1.0f);
        }

        void Float4x4::setScaling(float scalar)
        {
            setScaling(Float3(scalar));
        }

        void Float4x4::setScaling(const Float3 &vector)
        {
            _11 = vector.x;
            _22 = vector.y;
            _33 = vector.z;
        }

        void Float4x4::setEuler(const Float3 &euler)
        {
            setEuler(euler.x, euler.y, euler.z);
        }

        void Float4x4::setEuler(float x, float y, float z)
        {
            float cosX(std::cos(x));
            float sinX(std::sin(x));
            float cosY(std::cos(y));
            float sinY(std::sin(y));
            float cosZ(std::cos(z));
            float sinZ(std::sin(z));
            float cosXsinY(cosX * sinY);
            float sinXsinY(sinX * sinY);

            table[0][0] = (cosY * cosZ);
            table[1][0] = (-cosY * sinZ);
            table[2][0] = sinY;
            table[3][0] = 0.0f;

            table[0][1] = (sinXsinY * cosZ + cosX * sinZ);
            table[1][1] = (-sinXsinY * sinZ + cosX * cosZ);
            table[2][1] = (-sinX * cosY);
            table[3][1] = 0.0f;

            table[0][2] = (-cosXsinY * cosZ + sinX * sinZ);
            table[1][2] = (cosXsinY * sinZ + sinX * cosZ);
            table[2][2] = (cosX * cosY);
            table[3][2] = 0.0f;

            table[0][3] = 0.0f;
            table[1][3] = 0.0f;
            table[2][3] = 0.0f;
            table[3][3] = 1.0f;
        }

        void Float4x4::setRotation(const Float3 &axis, float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));

            table[0][0] = (cosAngle + axis.x * axis.x * (1.0f - cosAngle));
            table[0][1] = (axis.z * sinAngle + axis.y * axis.x * (1.0f - cosAngle));
            table[0][2] = (-axis.y * sinAngle + axis.z * axis.x * (1.0f - cosAngle));
            table[0][3] = 0.0f;

            table[1][0] = (-axis.z * sinAngle + axis.x * axis.y * (1.0f - cosAngle));
            table[1][1] = (cosAngle + axis.y * axis.y * (1.0f - cosAngle));
            table[1][2] = (axis.x * sinAngle + axis.z * axis.y * (1.0f - cosAngle));
            table[1][3] = 0.0f;

            table[2][0] = (axis.y * sinAngle + axis.x * axis.z * (1.0f - cosAngle));
            table[2][1] = (-axis.x * sinAngle + axis.y * axis.z * (1.0f - cosAngle));
            table[2][2] = (cosAngle + axis.z * axis.z * (1.0f - cosAngle));
            table[2][3] = 0.0f;

            table[3][0] = 0.0f;
            table[3][1] = 0.0f;
            table[3][2] = 0.0f;
            table[3][3] = 1.0f;
        }

        void Float4x4::setRotation(const Quaternion &rotation)
        {
            setRotation(rotation, Float3(0.0f, 0.0f, 0.0f));
        }

        void Float4x4::setRotation(const Quaternion &rotation, const Float3 &translation)
        {
            float xy(rotation.x * rotation.y);
            float zw(rotation.z * rotation.w);
            float xz(rotation.x * rotation.z);
            float yw(rotation.y * rotation.w);
            float yz(rotation.y * rotation.z);
            float xw(rotation.x * rotation.w);
            Float4 square(_mm_mul_ps(rotation.simd, rotation.simd));
            float determinant(1.0f / (square.x + square.y + square.z + square.w));
            
            this->rx.set((( square.x - square.y - square.z + square.w) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f);
            this->ry.set((2.0f * (xy - zw) * determinant), ((-square.x + square.y - square.z + square.w) * determinant), (2.0f * (yz + xw) * determinant), 0.0f);
            this->rz.set((2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-square.x - square.y + square.z + square.w) * determinant), 0.0f);
            this->translation = translation;
            this->tw = 1.0f;
        }

        void Float4x4::setRotationX(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));
            table[0][0] = 1.0f; table[0][1] = 0.0f;         table[0][2] = 0.0f;     table[0][3] = 0.0f;
            table[1][0] = 0.0f; table[1][1] = cosAngle;     table[1][2] = sinAngle; table[1][3] = 0.0f;
            table[2][0] = 0.0f; table[2][1] = -sinAngle;    table[2][2] = cosAngle; table[2][3] = 0.0f;
            table[3][0] = 0.0f; table[3][1] = 0.0f;         table[3][2] = 0.0f;     table[3][3] = 1.0f;
        }

        void Float4x4::setRotationY(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));
            table[0][0] = cosAngle;     table[0][1] = 0.0f; table[0][2] = -sinAngle;    table[0][3] = 0.0f;
            table[1][0] = 0.0f;         table[1][1] = 1.0f; table[1][2] = 0.0f;         table[1][3] = 0.0f;
            table[2][0] = sinAngle;     table[2][1] = 0.0f; table[2][2] = cosAngle;     table[2][3] = 0.0f;
            table[3][0] = 0.0f;         table[3][1] = 0.0f; table[3][2] = 0.0f;         table[3][3] = 1.0f;
        }

        void Float4x4::setRotationZ(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));
            table[0][0] = cosAngle;     table[0][1] = sinAngle; table[0][2] = 0.0f; table[0][3] = 0.0f;
            table[1][0] = -sinAngle;    table[1][1] = cosAngle; table[1][2] = 0.0f; table[1][3] = 0.0f;
            table[2][0] = 0.0f;         table[2][1] = 0.0f;     table[2][2] = 1.0f; table[2][3] = 0.0f;
            table[3][0] = 0.0f;         table[3][1] = 0.0f;     table[3][2] = 0.0f; table[3][3] = 1.0f;
        }

        void Float4x4::setOrthographic(float left, float top, float right, float bottom, float nearDepth, float farDepth)
        {
            table[0][0] = (2.0f / (right - left));
            table[0][1] = 0.0f;
            table[0][2] = 0.0f;
            table[0][3] = 0.0f;

            table[1][0] = 0.0f;
            table[1][1] = (2.0f / (top - bottom));
            table[1][2] = 0.0f;
            table[1][3] = 0.0f;

            table[2][0] = 0.0f;
            table[2][1] = 0.0f;
            table[2][2] = (-2.0f / (farDepth - nearDepth));
            table[2][3] = 0.0f;

            table[3][0] = -((right + left) / (right - left));;
            table[3][1] = -((top + bottom) / (top - bottom));
            table[3][2] = -((farDepth + nearDepth) / (farDepth - nearDepth));
            table[3][3] = 1.0f;
        }

        void Float4x4::setPerspective(float fieldOfView, float aspectRatio, float nearDepth, float farDepth)
        {
            float x(1.0f / std::tan(fieldOfView * 0.5f));
            float y(x * aspectRatio);
            float distance(farDepth - nearDepth);

            table[0][0] = x;
            table[0][1] = 0.0f;
            table[0][2] = 0.0f;
            table[0][3] = 0.0f;

            table[1][0] = 0.0f;
            table[1][1] = y;
            table[1][2] = 0.0f;
            table[1][3] = 0.0f;

            table[2][0] = 0.0f;
            table[2][1] = 0.0f;
            table[2][2] = ((farDepth + nearDepth) / distance);
            table[2][3] = 1.0f;

            table[3][0] = 0.0f;
            table[3][1] = 0.0f;
            table[3][2] = -((2.0f * farDepth * nearDepth) / distance);
            table[3][3] = 0.0f;
        }

        void Float4x4::setLookAt(const Float3 &source, const Float3 &target, const Float3 &worldUpVector)
        {
            nz.set((target - source).getNormal());
            nx.set(worldUpVector.cross(nz).getNormal());
            ny.set(nz.cross(nx).getNormal());
            nxw = 0.0f;
            nyw = 0.0f;
            nzw = 0.0f;

            rw.set(0.0f, 0.0f, 0.0f, 1.0f);

            invert();
        }

        void Float4x4::setLookAt(const Float3 &direction, const Float3 &worldUpVector)
        {
            nz.set(direction.getNormal());
            nx.set(worldUpVector.cross(nz).getNormal());
            ny.set(nz.cross(nx).getNormal());
            nxw = 0.0f;
            nyw = 0.0f;
            nzw = 0.0f;

            rw.set(0.0f, 0.0f, 0.0f, 1.0f);

            invert();
        }

        Float3 Float4x4::getEuler(void) const
        {
            Float3 euler;
            euler.y = std::asin(_31);

            float cosAngle = std::cos(euler.y);
            if (std::abs(cosAngle) > 0.005)
            {
                euler.x = std::atan2(-(_32 / cosAngle), (_33 / cosAngle));
                euler.z = std::atan2(-(_21 / cosAngle), (_11 / cosAngle));
            }
            else
            {
                euler.x = 0.0f;
                euler.y = std::atan2(_12, _22);
            }

            if (euler.x < 0.0f)
            {
                euler.x += (Pi * 2.0f);
            }

            if (euler.y < 0.0f)
            {
                euler.y += (Pi * 2.0f);
            }

            if (euler.z < 0.0f)
            {
                euler.z += (Pi * 2.0f);
            }

            return euler;
        }

        Float3 Float4x4::getScaling(void) const
        {
            return Float3(_11, _22, _33);
        }

        float Float4x4::getDeterminant(void) const
        {
            return ((table[0][0] * table[1][1] - table[1][0] * table[0][1]) *
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

        Float4x4 Float4x4::getTranspose(void) const
        {
            return Float4x4({ _11, _21, _31, _41,
                              _12, _22, _32, _42,
                              _13, _23, _33, _43,
                              _14, _24, _34, _44 });
        }

        Float4x4 Float4x4::getInverse(void) const
        {
            float determinant(getDeterminant());
            if (std::abs(determinant) < Epsilon)
            {
                return Float4x4();
            }
            else
            {
                determinant = (1.0f / determinant);

                Float4x4 matrix;
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

        Float4x4 Float4x4::getRotation(void) const
        {
            return Float4x4({ _11, _12, _13, 0,
                              _21, _22, _23, 0,
                              _31, _32, _33, 0,
                              0,   0,   0, 1 });
        }

        void Float4x4::transpose(void)
        {
            (*this) = getTranspose();
        }

        void Float4x4::invert(void)
        {
            (*this) = getInverse();
        }

        Float4 Float4x4::operator [] (int row) const
        {
            return rows[row];
        }

        Float4 &Float4x4::operator [] (int row)
        {
            return rows[row];
        }

        Float4x4::operator const float *() const
        {
            return data;
        }

        Float4x4::operator float *()
        {
            return data;
        }

        Float4x4 Float4x4::operator = (const Float4x4 &matrix)
        {
            rows[0] = matrix.rows[0];
            rows[1] = matrix.rows[1];
            rows[2] = matrix.rows[2];
            rows[3] = matrix.rows[3];
            return (*this);
        }

        Float4x4 Float4x4::operator = (const Quaternion &rotation)
        {
            setRotation(rotation);
            return (*this);
        }

        void Float4x4::operator *= (const Float4x4 &matrix)
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

        Float4x4 Float4x4::operator * (const Float4x4 &matrix) const
        {
            return Float4x4({ _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(simd[0], simd[0], _MM_SHUFFLE(0, 0, 0, 0)), matrix.simd[0]),
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
                                                    _mm_mul_ps(_mm_shuffle_ps(simd[3], simd[3], _MM_SHUFFLE(3, 3, 3, 3)), matrix.simd[3]))) });
        }

        Float3 Float4x4::operator * (const Float3 &vector) const
        {
            return Float3({ ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)) + _41,
                            ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)) + _42,
                            ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)) + _43 });
        }

        Float4 Float4x4::operator * (const Float4 &vector) const
        {
            return Float4({ ((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                            ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                            ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                            ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)) });
        }
    }; // namespace Math
}; // namespace Gek

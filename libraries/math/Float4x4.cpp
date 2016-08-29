#include "GEK\Math\Float4x4.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Float4x4 Float4x4::Identity({ 1.0f, 0.0f, 0.0f, 0.0f,
                                            0.0f, 1.0f, 0.0f, 0.0f,
                                            0.0f, 0.0f, 1.0f, 0.0f,
                                            0.0f, 0.0f, 0.0f, 1.0f });

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

        Math::Float4x4 &Float4x4::setIdentity(void)
        {
            simd[0] = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            simd[1] = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            simd[2] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            simd[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setRotationIdentity(void)
        {
            simd[0] = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
            simd[1] = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
            simd[2] = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setScaling(float scale)
        {
            _11 *= scale; _21 *= scale; _31 *= scale;
            _12 *= scale; _22 *= scale; _32 *= scale;
            _13 *= scale; _23 *= scale; _33 *= scale;
            return (*this);
        }

        Math::Float4x4 &Float4x4::setScaling(const Float3 &scale)
        {
            _11 *= scale.x; _21 *= scale.x; _31 *= scale.x;
            _12 *= scale.y; _22 *= scale.y; _32 *= scale.y;
            _13 *= scale.z; _23 *= scale.z; _33 *= scale.z;
            return (*this);
        }

        Math::Float4x4 &Float4x4::setTranslation(const Float3 &translation)
        {
            this->translation = translation;
            return (*this);
        }

        Math::Float4x4 &Float4x4::setEulerRotation(float pitch, float yaw, float roll)
        {
            float cosPitch(std::cos(pitch));
            float sinPitch(std::sin(pitch));
            float cosYaw(std::cos(yaw));
            float sinYaw(std::sin(yaw));
            float cosRoll(std::cos(roll));
            float sinRoll(std::sin(roll));
            float cosPitchSinYaw(cosPitch * sinYaw);
            float sinPitchSinYaw(sinPitch * sinYaw);

            rx.set((cosYaw * cosRoll), (-cosYaw * sinRoll), sinYaw, 0.0f);
            ry.set((sinPitchSinYaw * cosRoll + cosPitch * sinRoll), (-sinPitchSinYaw * sinRoll + cosPitch * cosRoll), (-sinPitch * cosYaw), 0.0f);
            rz.set((-cosPitchSinYaw * cosRoll + sinPitch * sinRoll), (cosPitchSinYaw * sinRoll + sinPitch * cosRoll), (cosPitch * cosYaw), 0.0f);
            rw.set(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setAngularRotation(const Float3 &axis, float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));

            rx.set((cosAngle + axis.x * axis.x * (1.0f - cosAngle)), (axis.z * sinAngle + axis.y * axis.x * (1.0f - cosAngle)), (-axis.y * sinAngle + axis.z * axis.x * (1.0f - cosAngle)), 0.0f);
            ry.set((-axis.z * sinAngle + axis.x * axis.y * (1.0f - cosAngle)), (cosAngle + axis.y * axis.y * (1.0f - cosAngle)), (axis.x * sinAngle + axis.z * axis.y * (1.0f - cosAngle)), 0.0f);
            rz.set((axis.y * sinAngle + axis.x * axis.z * (1.0f - cosAngle)), (-axis.x * sinAngle + axis.y * axis.z * (1.0f - cosAngle)), (cosAngle + axis.z * axis.z * (1.0f - cosAngle)), 0.0f);
            rw.set(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setPitchRotation(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));

            rx.set(1.0f, 0.0f, 0.0f, 0.0f);
            ry.set(0.0f, cosAngle, sinAngle, 0.0f);
            rz.set(0.0f, -sinAngle, cosAngle, 0.0f);
            rw.set(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setYawRotation(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));

            rx.set(cosAngle, 0.0f, -sinAngle, 0.0f);
            ry.set(0.0f, 1.0f, 0.0f, 0.0f);
            rz.set(sinAngle, 0.0f, cosAngle, 0.0f);
            rw.set(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setRollRotation(float radians)
        {
            float cosAngle(std::cos(radians));
            float sinAngle(std::sin(radians));

            rx.set(cosAngle, sinAngle, 0.0f, 0.0f);
            ry.set(-sinAngle, cosAngle, 0.0f, 0.0f);
            rz.set(0.0f, 0.0f, 1.0f, 0.0f);
            rw.set(0.0f, 0.0f, 0.0f, 1.0f);
            return (*this);
        }

        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
        Math::Float4x4 &Float4x4::setOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip)
        {
            rx.set((2.0f / (right - left)), 0.0f, 0.0f, 0.0f);
            ry.set(0.0f, (2.0f / (top - bottom)), 0.0f, 0.0f);
            rz.set(0.0f, 0.0f, (1.0f / (farClip - nearClip)), 0.0f);
            rw.set(((left + right) / (left - right)), ((top + bottom) / (bottom - top)), (nearClip / (nearClip - farClip)), 1.0f);
            return (*this);
        }

        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205350(v=vs.85).aspx
        Math::Float4x4 &Float4x4::setPerspective(float fieldOfView, float aspectRatio, float nearClip, float farClip)
        {
            float yScale(1.0f / std::tan(fieldOfView * 0.5f));
            float xScale(yScale / aspectRatio);
            float denominator(farClip - nearClip);

            rx.set(xScale, 0.0f, 0.0f, 0.0f);
            ry.set(0.0f, yScale, 0.0f, 0.0f);
            rz.set(0.0f, 0.0f, (farClip / denominator), 1.0f);
            rw.set(0.0f, 0.0f, ((-nearClip * farClip) / denominator), 0.0f);
            return (*this);
        }

        Math::Float4x4 &Float4x4::setLookAt(const Float3 &source, const Float3 &target, const Float3 &worldUpVector)
        {
            rz = (target - source).getNormal().w(0.0f);
            rx = worldUpVector.cross(nz).getNormal().w(0.0f);
            ry = nz.cross(nx).getNormal().w(0.0f);
            rw = source.w(1.0f);
            return (*this);
        }

        Quaternion Float4x4::getQuaternion(void) const
        {
            float trace(table[0][0] + table[1][1] + table[2][2] + 1.0f);
            if (trace > Math::Epsilon)
            {
                float denominator(0.5f / std::sqrt(trace));
                return Quaternion({
                    ((table[1][2] - table[2][1]) * denominator),
                    ((table[2][0] - table[0][2]) * denominator),
                    ((table[0][1] - table[1][0]) * denominator),
                    (0.25f / denominator),
                });
            }
            else
            {
                if ((table[0][0] > table[1][1]) && (table[0][0] > table[2][2]))
                {
                    float denominator(2.0f * std::sqrt(1.0f + table[0][0] - table[1][1] - table[2][2]));
                    return Quaternion({
                        (0.25f * denominator),
                        ((table[1][0] + table[0][1]) / denominator),
                        ((table[2][0] + table[0][2]) / denominator),
                        ((table[2][1] - table[1][2]) / denominator),
                    });
                }
                else if (table[1][1] > table[2][2])
                {
                    float denominator(2.0f * (std::sqrt(1.0f + table[1][1] - table[0][0] - table[2][2])));
                    return Quaternion({
                        ((table[1][0] + table[0][1]) / denominator),
                        (0.25f * denominator),
                        ((table[2][1] + table[1][2]) / denominator),
                        ((table[2][0] - table[0][2]) / denominator),
                    });
                }
                else
                {
                    float denominator(2.0f * (std::sqrt(1.0f + table[2][2] - table[0][0] - table[1][1])));
                    return Quaternion({
                        ((table[2][0] + table[0][2]) / denominator),
                        ((table[2][1] + table[1][2]) / denominator),
                        (0.25f * denominator),
                        ((table[1][0] - table[0][1]) / denominator),
                    });
                }
            }
        }

        Float3 Float4x4::getScaling(void) const
        {
            return Float3(
                Float3(_11, _21, _32).getLength(),
                Float3(_12, _22, _32).getLength(),
                Float3(_13, _23, _33).getLength()
            );
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
            __m128 tmp3, tmp2, tmp1, tmp0;
            tmp0 = _mm_shuffle_ps(simd[0], simd[1], 0x44);
            tmp2 = _mm_shuffle_ps(simd[0], simd[1], 0xEE);
            tmp1 = _mm_shuffle_ps(simd[2], simd[3], 0x44);
            tmp3 = _mm_shuffle_ps(simd[2], simd[3], 0xEE);
            return Float4x4(
            {
                _mm_shuffle_ps(tmp0, tmp1, 0x88),
                _mm_shuffle_ps(tmp0, tmp1, 0xDD),
                _mm_shuffle_ps(tmp2, tmp3, 0x88),
                _mm_shuffle_ps(tmp2, tmp3, 0xDD),
            });
        }

        Float4x4 Float4x4::getInverse(void) const
        {
            float determinant(getDeterminant());
            if (std::abs(determinant) < Epsilon)
            {
                return Identity;
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
            static const __m128 identity = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
            // Sets row/column 4 to identity
            return Float4x4(
            { 
                _mm_setr_ps(rows[0].x, rows[0].y, rows[0].z, 0.0f),
                _mm_setr_ps(rows[1].x, rows[1].y, rows[1].z, 0.0f),
                _mm_setr_ps(rows[2].x, rows[2].y, rows[2].z, 0.0f),
                identity,
            });
        }

        Math::Float4x4 &Float4x4::transpose(void)
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

        Math::Float4x4 &Float4x4::invert(void)
        {
            (*this) = getInverse();
            return (*this);
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
            return Float3({ ((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                            ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                            ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)) });
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

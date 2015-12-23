#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Quaternion Quaternion::createIdentity(void)
        {
            return Quaternion({ 0.0f, 0.0f, 0.0f, 1.0f });
        }

        Quaternion Quaternion::createEuler(float pitch, float yaw, float roll)
        {
            float sinPitch(std::sin(pitch * 0.5f));
            float sinYaw(std::sin(yaw * 0.5f));
            float sinRoll(std::sin(roll * 0.5f));
            float cosPitch(std::cos(pitch * 0.5f));
            float cosYaw(std::cos(yaw * 0.5f));
            float cosRoll(std::cos(roll * 0.5f));
            return Quaternion({
                ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)),
            });
        }

        Quaternion Quaternion::createRotation(const Float3 &axis, float radians)
        {
            Float3 normal(axis.getNormal());
            float sinAngle(std::sin(radians * 0.5f));
            return Quaternion({
                (normal.x * sinAngle),
                (normal.y * sinAngle),
                (normal.z * sinAngle),
                std::cos(radians * 0.5f),
            });
        }

        Quaternion Quaternion::createRotation(const Float4x4 &rotation)
        {
            float trace(rotation.table[0][0] + rotation.table[1][1] + rotation.table[2][2] + 1.0f);
            if (trace > Math::Epsilon)
            {
                float denominator(0.5f / std::sqrt(trace));
                return Quaternion({
                    ((rotation.table[1][2] - rotation.table[2][1]) * denominator),
                    ((rotation.table[2][0] - rotation.table[0][2]) * denominator),
                    ((rotation.table[0][1] - rotation.table[1][0]) * denominator),
                    (0.25f / denominator),
                });
            }
            else
            {
                if ((rotation.table[0][0] > rotation.table[1][1]) && (rotation.table[0][0] > rotation.table[2][2]))
                {
                    float denominator(2.0f * std::sqrt(1.0f + rotation.table[0][0] - rotation.table[1][1] - rotation.table[2][2]));
                    return Quaternion({
                        (0.25f * denominator),
                        ((rotation.table[1][0] + rotation.table[0][1]) / denominator),
                        ((rotation.table[2][0] + rotation.table[0][2]) / denominator),
                        ((rotation.table[2][1] - rotation.table[1][2]) / denominator),
                    });
                }
                else if (rotation.table[1][1] > rotation.table[2][2])
                {
                    float denominator(2.0f * (std::sqrt(1.0f + rotation.table[1][1] - rotation.table[0][0] - rotation.table[2][2])));
                    return Quaternion({
                        ((rotation.table[1][0] + rotation.table[0][1]) / denominator),
                        (0.25f * denominator),
                        ((rotation.table[2][1] + rotation.table[1][2]) / denominator),
                        ((rotation.table[2][0] - rotation.table[0][2]) / denominator),
                    });
                }
                else
                {
                    float denominator(2.0f * (std::sqrt(1.0f + rotation.table[2][2] - rotation.table[0][0] - rotation.table[1][1])));
                    return Quaternion({
                        ((rotation.table[2][0] + rotation.table[0][2]) / denominator),
                        ((rotation.table[2][1] + rotation.table[1][2]) / denominator),
                        (0.25f * denominator),
                        ((rotation.table[1][0] - rotation.table[0][1]) / denominator),
                    });
                }
            }
        }

        float Quaternion::getLengthSquared(void) const
        {
            return this->dot(*this);
        }

        float Quaternion::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        Quaternion Quaternion::getNormal(void) const
        {
            return reinterpret_cast<Quaternion &>(_mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength()))));
        }

        Quaternion Quaternion::getInverse(void) const
        {
            return Quaternion(-x, -y, -z, w);
        }

        void Quaternion::invert(void)
        {
            x *= -1.0f;
            y *= -1.0f;
            z *= -1.0f;
        }

        void Quaternion::normalize(void)
        {
            (*this).set(getNormal());
        }

        float Quaternion::dot(const Quaternion &rotation) const
        {
            return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
        }

        Quaternion Quaternion::lerp(const Quaternion &rotation, float factor) const
        {
            return Quaternion(Math::lerp(x, rotation.x, factor),
                              Math::lerp(y, rotation.y, factor),
                              Math::lerp(z, rotation.z, factor),
                              Math::lerp(w, rotation.w, factor));
        }

        Quaternion Quaternion::slerp(const Quaternion &rotation, float factor) const
        {
            Quaternion normalA(getNormal());
            Quaternion normalB(rotation.getNormal());
            float dot(normalA.dot(normalB));
            if (dot < 0.0f)
            {
                dot = -dot;
                normalB.x = -normalB.x;
                normalB.y = -normalB.y;
                normalB.z = -normalB.z;
                normalB.w = -normalB.w;
            }

            if (dot < 1.0f)
            {
                float theta = std::acos(dot);
                float sinTheta = std::sin(theta);
                float factorA = (std::sin((1.0f - factor) * theta) / sinTheta);
                float factorB = (std::sin(factor * theta) / sinTheta);
                return Quaternion(Math::blend(normalA.x, normalB.x, factorA, factorB),
                                  Math::blend(normalA.y, normalB.y, factorA, factorB),
                                  Math::blend(normalA.z, normalB.z, factorA, factorB),
                                  Math::blend(normalA.w, normalB.w, factorA, factorB));
            }
            else
            {
                return lerp(rotation, factor);
            }
        }

        bool Quaternion::operator == (const Quaternion &rotation) const
        {
            if (x != rotation.x) return false;
            if (y != rotation.y) return false;
            if (z != rotation.z) return false;
            if (w != rotation.w) return false;
            return true;
        }

        bool Quaternion::operator != (const Quaternion &rotation) const
        {
            if (x != rotation.x) return true;
            if (y != rotation.y) return true;
            if (z != rotation.z) return true;
            if (w != rotation.w) return true;
            return false;
        }

        Float3 Quaternion::operator * (const Float3 &vector) const
        {
            float x2(x * 2.0f);
            float y2(y * 2.0f);
            float z2(z * 2.0f);
            float xx2(x * x2);
            float yy2(y * y2);
            float zz2(z * z2);
            float xy2(x * y2);
            float xz2(x * z2);
            float yz2(y * z2);
            float wx2(w * x2);
            float wy2(w * y2);
            float wz2(w * z2);
            return Float3(((1.0f - (yy2 + zz2)) * vector.x + (xy2 - wz2) * vector.y + (xz2 + wy2) * vector.z),
                          ((xy2 + wz2) * vector.x + (1.0f - (xx2 + zz2)) * vector.y + (yz2 - wx2) * vector.z),
                          ((xz2 - wy2) * vector.x + (yz2 + wx2) * vector.y + (1.0f - (xx2 + yy2)) * vector.z));
        }

        Quaternion Quaternion::operator * (const Quaternion &rotation) const
        {
            return Quaternion(((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                              ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                              ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                              ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z))).getNormal();
        }

        void Quaternion::operator *= (const Quaternion &rotation)
        {
            (*this) = ((*this) * rotation);
        }

        Quaternion Quaternion::operator = (const Quaternion &rotation)
        {
            simd = rotation.simd;
            return (*this);
        }

        Quaternion Quaternion::operator = (const Float4x4 &rotation)
        {
            (*this) = createRotation(rotation);
            return (*this);
        }
    }; // namespace Math
}; // namespace Gek

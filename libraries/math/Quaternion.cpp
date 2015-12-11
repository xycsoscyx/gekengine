#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Quaternion::Quaternion(const Float4x4 &rotation)
        {
            setRotation(rotation);
        }

        Quaternion::Quaternion(const Float3 &axis, float radians)
        {
            setRotation(axis, radians);
        }

        Quaternion::Quaternion(const Float3 &euler)
        {
            setEuler(euler.x, euler.y, euler.z);
        }

        Quaternion::Quaternion(float x, float y, float z)
        {
            setEuler(x, y, z);
        }

        void Quaternion::setIdentity(void)
        {
            x = y = z = 0.0f;
            w = 1.0f;
        }

        void Quaternion::setEuler(const Float3 &euler)
        {
            setEuler(euler.x, euler.y, euler.z);
        }

        void Quaternion::setEuler(float x, float y, float z)
        {
            float sinX(std::sin(x * 0.5f));
            float sinY(std::sin(y * 0.5f));
            float sinZ(std::sin(z * 0.5f));
            float cosX(std::cos(x * 0.5f));
            float cosY(std::cos(y * 0.5f));
            float cosZ(std::cos(z * 0.5f));
            this->x = ((sinX * cosY * cosZ) - (cosX * sinY * sinZ));
            this->y = ((sinX * cosY * sinZ) + (cosX * sinY * cosZ));
            this->z = ((cosX * cosY * sinZ) - (sinX * sinY * cosZ));
            this->w = ((cosX * cosY * cosZ) + (sinX * sinY * sinZ));
            normalize();
        }

        void Quaternion::setRotation(const Float3 &axis, float radians)
        {
            Float3 normal(axis.getNormal());
            float sinAngle(std::sin(radians * 0.5f));
            x = (normal.x * sinAngle);
            y = (normal.y * sinAngle);
            z = (normal.z * sinAngle);
            w = std::cos(radians * 0.5f);
            normalize();
        }

        void Quaternion::setRotation(const Float4x4 &rotation)
        {
            float trace(rotation.table[0][0] + rotation.table[1][1] + rotation.table[2][2] + 1.0f);
            if (trace > Math::Epsilon)
            {
                float denominator(0.5f / std::sqrt(trace));
                w = (0.25f / denominator);
                x = ((rotation.table[1][2] - rotation.table[2][1]) * denominator);
                y = ((rotation.table[2][0] - rotation.table[0][2]) * denominator);
                z = ((rotation.table[0][1] - rotation.table[1][0]) * denominator);
            }
            else
            {
                if ((rotation.table[0][0] > rotation.table[1][1]) && (rotation.table[0][0] > rotation.table[2][2]))
                {
                    float denominator(2.0f * std::sqrt(1.0f + rotation.table[0][0] - rotation.table[1][1] - rotation.table[2][2]));
                    x = (0.25f * denominator);
                    y = ((rotation.table[1][0] + rotation.table[0][1]) / denominator);
                    z = ((rotation.table[2][0] + rotation.table[0][2]) / denominator);
                    w = ((rotation.table[2][1] - rotation.table[1][2]) / denominator);
                }
                else if (rotation.table[1][1] > rotation.table[2][2])
                {
                    float denominator(2.0f * (std::sqrt(1.0f + rotation.table[1][1] - rotation.table[0][0] - rotation.table[2][2])));
                    x = ((rotation.table[1][0] + rotation.table[0][1]) / denominator);
                    y = (0.25f * denominator);
                    z = ((rotation.table[2][1] + rotation.table[1][2]) / denominator);
                    w = ((rotation.table[2][0] - rotation.table[0][2]) / denominator);
                }
                else
                {
                    float denominator(2.0f * (std::sqrt(1.0f + rotation.table[2][2] - rotation.table[0][0] - rotation.table[1][1])));
                    x = ((rotation.table[2][0] + rotation.table[0][2]) / denominator);
                    y = ((rotation.table[2][1] + rotation.table[1][2]) / denominator);
                    z = (0.25f * denominator);
                    w = ((rotation.table[1][0] - rotation.table[0][1]) / denominator);
                }
            }

            normalize();
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

        Float3 Quaternion::getEuler(void) const
        {
            Float4 square((*this) * (*this));
            return Float3(std::atan2(2.0f * ((y * z) + (x * w)), (-square.x - square.y + square.z + square.w)),
                std::asin(-2.0f * ((x * z) - (y * w))),
                std::atan2(2.0f * ((x * y) + (z * w)), (square.x - square.y - square.z + square.w)));
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
            Quaternion adjustedRotation(rotation);
            float dot(this->dot(rotation));
            if (dot < 0.0f)
            {
                dot = -dot;
                adjustedRotation.x = -adjustedRotation.x;
                adjustedRotation.y = -adjustedRotation.y;
                adjustedRotation.z = -adjustedRotation.z;
                adjustedRotation.w = -adjustedRotation.w;
            }

            if (dot < 1.0f)
            {
                float theta = std::acos(dot);
                float sinTheta = std::sin(theta);
                float factorA = (std::sin((1.0f - factor) * theta) / sinTheta);
                float factorB = (std::sin(factor * theta) / sinTheta);
                return Quaternion(Math::blend(x, adjustedRotation.x, factorA, factorB),
                    Math::blend(y, adjustedRotation.y, factorA, factorB),
                    Math::blend(z, adjustedRotation.z, factorA, factorB),
                    Math::blend(w, adjustedRotation.w, factorA, factorB));
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
            setRotation(rotation);
            return (*this);
        }
    }; // namespace Math
}; // namespace Gek

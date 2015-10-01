#pragma once

#include <algorithm>
#include "GEK\Math\Common.h"
#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Math
    {
        struct Float4x4;

        struct Quaternion : public Float4
        {
        public:
            Quaternion(void)
                : Float4(0.0f, 0.0f, 0.0f, 1.0f)
            {
            }

            Quaternion(__m128 data)
                : Float4(data)
            {
            }

            Quaternion(const float(&data)[4])
                : Float4({ data[0], data[1], data[2], data[3] })
            {
            }

            Quaternion(const float *data)
                : Float4(data)
            {
            }

            Quaternion(const Float4 &vector)
                : Float4(vector)
            {
            }

            Quaternion(float x, float y, float z, float w)
                : Float4(x, y, z, w)
            {
            }

            Quaternion(const Float4x4 &rotation)
            {
                setRotation(rotation);
            }

            Quaternion(const Float3 &axis, float radians)
            {
                setRotation(axis, radians);
            }

            Quaternion(const Float3 &euler)
            {
                setEuler(euler.x, euler.y, euler.z);
            }

            Quaternion(float x, float y, float z)
            {
                setEuler(x, y, z);
            }

            void setIdentity(void)
            {
                x = y = z = 0.0f;
                w = 1.0f;
            }

            void setLength(float length)
            {
                length = (length / getLength());
                x *= length;
                y *= length;
                z *= length;
                w *= length;
            }

            void setEuler(const Float3 &euler)
            {
                setEuler(euler.x, euler.y, euler.z);
            }

            void setEuler(float x, float y, float z)
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
            }

            void setRotation(const Float3 &axis, float radians)
            {
                Float3 normal(axis.getNormal());
                float sinAngle(std::sin(radians * 0.5f));
                x = (normal.x * sinAngle);
                y = (normal.y * sinAngle);
                z = (normal.z * sinAngle);
                w = std::cos(radians * 0.5f);
            }

            void setRotation(const Float4x4 &rotation);

            Float3 getEuler(void) const
            {
                Float4 square((*this) * (*this));
                return Float3(std::atan2(2.0f * ((y * z) + (x * w)), (-square.x - square.y + square.z + square.w)),
                              std::asin(-2.0f * ((x * z) - (y * w))),
                              std::atan2(2.0f * ((x * y) + (z * w)), (square.x - square.y - square.z + square.w)));
            }

            Quaternion getInverse(void) const
            {
                return Quaternion(-x, -y, -z, w);
            }

            void invert(void)
            {
                (*this) *= Float4({ -1.0f, -1.0f, -1.0f, 1.0f });
            }

            Quaternion slerp(const Quaternion &rotation, float factor) const
            {
                Quaternion adjustedRotation(rotation);
                float cosAngle(rotation.dot(*this));
                if (cosAngle < 0.0f)
                {
                    cosAngle = -cosAngle;
                    adjustedRotation.x = -adjustedRotation.x;
                    adjustedRotation.y = -adjustedRotation.y;
                    adjustedRotation.z = -adjustedRotation.z;
                    adjustedRotation.w = -adjustedRotation.w;
                }

                if (std::abs(1.0f - cosAngle) < Epsilon)
                {
                    return blend((*this), adjustedRotation, factor);
                }
                else
                {
                    float omega(std::acos(cosAngle));
                    float sinOmega(std::sin(omega));
                    float factorA = (std::sin((1.0f - factor) * omega) / sinOmega);
                    float factorB = (std::sin(factor * omega) / sinOmega);
                    return Quaternion(((factorA * x) + (factorB * adjustedRotation.x)),
                                      ((factorA * y) + (factorB * adjustedRotation.y)),
                                      ((factorA * z) + (factorB * adjustedRotation.z)),
                                      ((factorA * w) + (factorB * adjustedRotation.w)));
                }
            }

            Float3 operator * (const Float3 &vector) const
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

            Quaternion operator * (const Quaternion &rotation) const
            {
                return Quaternion(((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                                  ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                                  ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                                  ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z)));
            }

            void operator *= (const Quaternion &rotation)
            {
                (*this) = ((*this) * rotation);
            }

            Quaternion operator = (const Float4 &vector)
            {
                simd = vector.simd;
                return (*this);
            }

            Quaternion operator = (const Quaternion &rotation)
            {
                simd = rotation.simd;
                return (*this);
            }

            Quaternion operator = (const Float4x4 &rotation)
            {
                setRotation(rotation);
                return (*this);
            }
        };
    }; // namespace Math
}; // namespace Gek

#include "GEK\Math\Matrix4x4.h"

namespace Gek
{
    namespace Math
    {
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
    }; // namespace Math
}; // namespace Gek

#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Matrix4x4.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        void Quaternion::setEulerRotation(float pitch, float yaw, float roll)
        {
            float sinPitch(std::sin(pitch * 0.5f));
            float sinYaw(std::sin(yaw * 0.5f));
            float sinRoll(std::sin(roll * 0.5f));
            float cosPitch(std::cos(pitch * 0.5f));
            float cosYaw(std::cos(yaw * 0.5f));
            float cosRoll(std::cos(roll * 0.5f));
            x = ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll));
            y = ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll));
            z = ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll));
            w = ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll));
        }

        void Quaternion::setAngularRotation(const Float3 &axis, float radians)
        {
            Float3 normal(axis.getNormal());
            float sinAngle(std::sin(radians * 0.5f));
            x = (normal.x * sinAngle);
            y = (normal.y * sinAngle);
            z = (normal.z * sinAngle);
            w = std::cos(radians * 0.5f);
        }

        Float4x4 Quaternion::getMatrix(const Float3 &translation) const
        {
            float xx(x * x);
            float yy(y * y);
            float zz(z * z);
            float ww(w * w);
            float length(xx + yy + zz + ww);
            if (length == 0.0f)
            {
                return Float4x4({ 1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f,
                                  translation.x, translation.y, translation.z, 1.0f });
            }
            else
            {
                float determinant(1.0f / length);
                float xy(x * y);
                float xz(x * z);
                float xw(x * w);
                float yz(y * z);
                float yw(y * w);
                float zw(z * w);
                return Float4x4({ ((xx - yy - zz + ww) * determinant), (2.0f * (xy + zw) * determinant), (2.0f * (xz - yw) * determinant), 0.0f,
                                   (2.0f * (xy - zw) * determinant), ((-xx + yy - zz + ww) * determinant), (2.0f * (yz + xw) * determinant), 0.0f,
                                   (2.0f * (xz + yw) * determinant), (2.0f * (yz - xw) * determinant), ((-xx - yy + zz + ww) * determinant), 0.0f,
                                    translation.x, translation.y, translation.z, 1.0f });
            }
        }

        float Quaternion::getLengthSquared(void) const
        {
            return dot(*this);
        }

        float Quaternion::getLength(void) const
        {
            return std::sqrt(getLengthSquared());
        }

        Quaternion Quaternion::getNormal(void) const
        {
            return _mm_mul_ps(simd, _mm_rcp_ps(_mm_set1_ps(getLength())));
        }

        Quaternion Quaternion::getInverse(void) const
        {
            return Quaternion(-x, -y, -z, w);
        }

        void Quaternion::invert(void)
        {
            x = -x;
            y = -y;
            z = -z;
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
            return Math::lerp((*this), rotation, factor);
        }

        Quaternion Quaternion::slerp(const Quaternion &rotation, float factor) const
        {
            Quaternion normalA(getNormal());
            Quaternion normalB(rotation.getNormal());
            float dot(normalA.dot(normalB));
            if (dot < 0.0f)
            {
                dot = -dot;
                normalB *= -1.0f;
            }

            if (dot < 1.0f)
            {
                float theta = std::acos(dot);
                float sinTheta = std::sin(theta);
                float factorA = (std::sin((1.0f - factor) * theta) / sinTheta);
                float factorB = (std::sin(factor * theta) / sinTheta);
                return Math::blend(normalA, factorA, normalB, factorB);
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
    }; // namespace Math
}; // namespace Gek

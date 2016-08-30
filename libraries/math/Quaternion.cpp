#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);

        Quaternion Quaternion::createEulerRotation(float pitch, float yaw, float roll)
        {
            float sinPitch(std::sin(pitch * 0.5f));
            float sinYaw(std::sin(yaw * 0.5f));
            float sinRoll(std::sin(roll * 0.5f));
            float cosPitch(std::cos(pitch * 0.5f));
            float cosYaw(std::cos(yaw * 0.5f));
            float cosRoll(std::cos(roll * 0.5f));
            return Quaternion(
            {
                ((sinPitch * cosYaw * cosRoll) - (cosPitch * sinYaw * sinRoll)),
                ((sinPitch * cosYaw * sinRoll) + (cosPitch * sinYaw * cosRoll)),
                ((cosPitch * cosYaw * sinRoll) - (sinPitch * sinYaw * cosRoll)),
                ((cosPitch * cosYaw * cosRoll) + (sinPitch * sinYaw * sinRoll)),
            });
        }

        Quaternion Quaternion::createAngularRotation(const Float3 &axis, float radians)
        {
            float halfRadians = (radians * 0.5f);
            Float3 normal(axis.getNormal());
            float sinAngle(std::sin(halfRadians));
            return Quaternion(
            {
                (normal.x * sinAngle),
                (normal.y * sinAngle),
                (normal.z * sinAngle),
                std::cos(halfRadians),
            });
        }

        Float4x4 Quaternion::getMatrix(void) const
        {
            float xx(x * x);
            float yy(y * y);
            float zz(z * z);
            float ww(w * w);
            float length(xx + yy + zz + ww);
            if (length == 0.0f)
            {
                return Float4x4::Identity;
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
                                    0.0f, 0.0f, 0.0f, 1.0f });
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
            (*this) = getNormal();
        }

        float Quaternion::dot(const Quaternion &rotation) const
        {
            return ((x * rotation.x) + (y * rotation.y) + (z * rotation.z) + (w * rotation.w));
        }

        Quaternion Quaternion::slerp(const Quaternion &rotation, float factor) const
        {
            float omega = std::acos(saturate(dot(rotation), -1.0f, 1.0f));
            if (std::abs(omega) < 1e-10f)
            {
                omega = 1e-10f;
            }

            float sinTheta = std::sin(omega);
            float factor0 = (std::sin((1.0f - factor) * omega) / sinTheta);
            float factor1 = (std::sin(factor * omega) / sinTheta);
            return blend((*this), factor0, rotation, factor1);
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
            Float3 cross(2.0f * this->vector.cross(vector));
            return (vector + (this->w * cross) + this->vector.cross(cross));
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

#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector4.h"
#include <Math.h>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector3;
        template <typename TYPE> struct BaseVector4;
        template <typename TYPE> struct BaseMatrix4x4;

        template <typename TYPE>
        struct BaseQuaternion : public BaseVector4<TYPE>
        {
        public:
            BaseQuaternion(void)
                : BaseVector4()
            {
            }

            BaseQuaternion(const BaseVector4<TYPE> &vector)
                : BaseVector4(vector)
            {
            }

            BaseQuaternion(TYPE x, TYPE y, TYPE z, TYPE w)
                : BaseVector4(x, y, z, w)
            {
            }

            BaseQuaternion(const BaseMatrix4x4<TYPE> &rotation)
            {
                SeBaseMatrix(rotation);
            }

            BaseQuaternion(const BaseVector3<TYPE> &axis, TYPE radians)
            {
                setRotation(axis, radians);
            }

            BaseQuaternion(TYPE x, TYPE y, TYPE z)
            {
                setEuler(x, y, z);
            }

            BaseQuaternion(const BaseVector3<TYPE> &euler)
            {
                setEuler(euler);
            }

            void setIdentity(void)
            {
                x = y = z = TYPE(0);
                w = TYPE(1);
            }

            void setLength(TYPE length)
            {
                length = (length / getLength());
                x *= length;
                y *= length;
                z *= length;
                w *= length;
            }

            void setEuler(const BaseVector3<TYPE> &euler)
            {
                setEuler(euler.x, euler.y, euler.z);
            }

            void setEuler(TYPE x, TYPE y, TYPE z)
            {
                TYPE sinX(sin(x * TYPE(0.5)));
                TYPE sinY(sin(y * TYPE(0.5)));
                TYPE sinZ(sin(z * TYPE(0.5)));
                TYPE cosX(cos(x * TYPE(0.5)));
                TYPE cosY(cos(y * TYPE(0.5)));
                TYPE cosZ(cos(z * TYPE(0.5)));
                this->x = ((sinX * cosY * cosZ) - (cosX * sinY * sinZ));
                this->y = ((sinX * cosY * sinZ) + (cosX * sinY * cosZ));
                this->z = ((cosX * cosY * sinZ) - (sinX * sinY * cosZ));
                this->w = ((cosX * cosY * cosZ) + (sinX * sinY * sinZ));
            }

            void setRotation(const BaseVector3<TYPE> &axis, TYPE radians)
            {
                BaseVector3<TYPE> normal(axis.getNormal());
                TYPE sinAngle(sin(radians * TYPE(0.5)));
                x = (normal.x * sinAngle);
                y = (normal.y * sinAngle);
                z = (normal.z * sinAngle);
                w = cos(radians * TYPE(0.5));
            }

            void setRotation(const BaseMatrix4x4<TYPE> &rotation)
            {
                TYPE trace(rotation.table[0][0] + rotation.table[1][1] + rotation.table[2][2] + TYPE(1));
                if (trace > _EPSILON)
                {
                    TYPE denominator(TYPE(0.5) / sqrt(trace));
                    w = (TYPE(0.25) / denominator);
                    x = ((rotation.table[1][2] - rotation.table[2][1]) * denominator);
                    y = ((rotation.table[2][0] - rotation.table[0][2]) * denominator);
                    z = ((rotation.table[0][1] - rotation.table[1][0]) * denominator);
                }
                else
                {
                    if ((rotation.table[0][0] > rotation.table[1][1]) && (rotation.table[0][0] > rotation.table[2][2]))
                    {
                        TYPE denominator(TYPE(2) * sqrt(TYPE(1) + rotation.table[0][0] - rotation.table[1][1] - rotation.table[2][2]));
                        x = (TYPE(0.25) * denominator);
                        y = ((rotation.table[1][0] + rotation.table[0][1]) / denominator);
                        z = ((rotation.table[2][0] + rotation.table[0][2]) / denominator);
                        w = ((rotation.table[2][1] - rotation.table[1][2]) / denominator);
                    }
                    else if (rotation.table[1][1] > rotation.table[2][2])
                    {
                        TYPE denominator(TYPE(2) * (sqrt(TYPE(1) + rotation.table[1][1] - rotation.table[0][0] - rotation.table[2][2])));
                        x = ((rotation.table[1][0] + rotation.table[0][1]) / denominator);
                        y = (TYPE(0.25) * denominator);
                        z = ((rotation.table[2][1] + rotation.table[1][2]) / denominator);
                        w = ((rotation.table[2][0] - rotation.table[0][2]) / denominator);
                    }
                    else
                    {
                        TYPE denominator(TYPE(2) * (sqrt(TYPE(1) + rotation.table[2][2] - rotation.table[0][0] - rotation.table[1][1])));
                        x = ((rotation.table[2][0] + rotation.table[0][2]) / denominator);
                        y = ((rotation.table[2][1] + rotation.table[1][2]) / denominator);
                        z = (TYPE(0.25) * denominator);
                        w = ((rotation.table[1][0] - rotation.table[0][1]) / denominator);
                    }
                }

                normalize();
            }

            BaseVector3<TYPE> getEuler(void) const
            {
                TYPE squareX(x * x);
                TYPE squareY(y * y);
                TYPE squareZ(z * z);
                TYPE squareW(w * w);
                return BaseVector3<TYPE>(TYPE(atan2(TYPE(2) * ((y * z) + (x * w)), (-squareX - squareY + squareZ + squareW))),
                                         TYPE(asin(-TYPE(2) * ((x * z) - (y * w)))),
                                         TYPE(atan2(TYPE(2) * ((x * y) + (z * w)), (squareX - squareY - squareZ + squareW))));
            }

            BaseMatrix4x4<TYPE> getMatrix(void) const
            {
                return BaseMatrix4x4<TYPE>(*this);
            }

            BaseQuaternion<TYPE> getNormal(void) const
            {
                TYPE inverseLength(TYPE(1) / getLength());
                return BaseQuaternion<TYPE>((x * inverseLength), (y * inverseLength), (z * inverseLength), (w * inverseLength));
            }

            BaseQuaternion<TYPE> getInverse(void) const
            {
                return BaseQuaternion<TYPE>(-x, -y, -z, w);
            }

            void normalize(void)
            {
                TYPE inverseLength(TYPE(1) / getLength());
                x *= inverseLength;
                y *= inverseLength;
                z *= inverseLength;
                w *= inverseLength;
            }

            void invert(void)
            {
                x = -x;
                y = -y;
                z = -z;
                w = w;
            }

            BaseQuaternion<TYPE> slerp(const BaseQuaternion<TYPE> &rotation, TYPE factor) const
            {
                BaseQuaternion<TYPE> adjustedRotation(rotation);
                TYPE cosAngle(rotation.Dot(*this));
                if (cosAngle < TYPE(0))
                {
                    cosAngle = -cosAngle;
                    adjustedRotation.x = -adjustedRotation.x;
                    adjustedRotation.y = -adjustedRotation.y;
                    adjustedRotation.z = -adjustedRotation.z;
                    adjustedRotation.w = -adjustedRotation.w;
                }

                if (abs(TYPE(1) - cosAngle) < Epsilon)
                {
                    return blend((*this), adjustedRotation, factor);
                }
                else
                {
                    TYPE omega(acos(cosAngle));
                    TYPE sinOmega(sin(omega));
                    TYPE factorA = (sin((TYPE(1) - factor) * omega) / sinOmega);
                    TYPE factorB = (sin(factor * omega) / sinOmega);
                    return BaseQuaternion<TYPE>(((factorA * x) + (factorB * adjustedRotation.x)),
                                                ((factorA * y) + (factorB * adjustedRotation.y)),
                                                ((factorA * z) + (factorB * adjustedRotation.z)),
                                                ((factorA * w) + (factorB * adjustedRotation.w)));
                }
            }

            BaseVector3<TYPE> operator * (const BaseVector3<TYPE> &vector) const
            {
                TYPE x2(x * TYPE(2));
                TYPE y2(y * TYPE(2));
                TYPE z2(z * TYPE(2));
                TYPE xx2(x * x2);
                TYPE yy2(y * y2);
                TYPE zz2(z * z2);
                TYPE xy2(x * y2);
                TYPE xz2(x * z2);
                TYPE yz2(y * z2);
                TYPE wx2(w * x2);
                TYPE wy2(w * y2);
                TYPE wz2(w * z2);
                return BaseVector3<TYPE>(((TYPE(1) - (yy2 + zz2)) * vector.x + (xy2 - wz2) * vector.y + (xz2 + wy2) * vector.z),
                                         ((xy2 + wz2) * vector.x + (TYPE(1) - (xx2 + zz2)) * vector.y + (yz2 - wx2) * vector.z),
                                         ((xz2 - wy2) * vector.x + (yz2 + wx2) * vector.y + (TYPE(1) - (xx2 + yy2)) * vector.z));
            }

            BaseQuaternion<TYPE> operator * (const BaseQuaternion<TYPE> &rotation) const
            {
                return BaseQuaternion<TYPE>(((w * rotation.x) + (x * rotation.w) + (y * rotation.z) - (z * rotation.y)),
                                            ((w * rotation.y) + (y * rotation.w) + (z * rotation.x) - (x * rotation.z)),
                                            ((w * rotation.z) + (z * rotation.w) + (x * rotation.y) - (y * rotation.x)),
                                            ((w * rotation.w) - (x * rotation.x) - (y * rotation.y) - (z * rotation.z)));
            }

            void operator *= (const BaseQuaternion<TYPE> &rotation)
            {
                (*this) = ((*this) * rotation);
            }

            BaseQuaternion<TYPE> operator = (const BaseVector4<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = vector.w;
                return (*this);
            }

            BaseQuaternion<TYPE> operator = (const BaseQuaternion<TYPE> &rotation)
            {
                x = rotation.x;
                y = rotation.y;
                z = rotation.z;
                w = rotation.w;
                return (*this);
            }

            BaseQuaternion<TYPE> operator = (const BaseMatrix4x4<TYPE> &rotation)
            {
                setRotation(rotation);
                return (*this);
            }
        };

        typedef BaseQuaternion<float> Quaternion;
    }; // namespace Math
}; // namespace Gek

#pragma once

#include <Math.h>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector3;
        template <typename TYPE> struct BaseVector4;
        template <typename TYPE> struct BaseQuaternion;

        template <typename TYPE>
        struct BaseMatrix4x4
        {
        public:
            union
            {
                struct
                {
                    TYPE data[16];
                };

                struct
                {
                    TYPE table[4][4]; 
                };

                struct
                {
                    TYPE _11, _12, _13, _14;
                    TYPE _21, _22, _23, _24;
                    TYPE _31, _32, _33, _34;
                    TYPE _41, _42, _43, _44;
                };

                struct
                {
                    BaseVector4<TYPE> rx;
                    BaseVector4<TYPE> ry;
                    BaseVector4<TYPE> rz;
                    BaseVector4<TYPE> translation;
                };

                struct
                {
                    BaseVector4<TYPE> rows[4];
                };
            };

        public:
            BaseMatrix4x4(void)
            {
                setIdentity();
            }

            BaseMatrix4x4(const TYPE *data)
            {
                memcpy(this->data, data, sizeof(this->data));
            }

            BaseMatrix4x4(const BaseMatrix4x4 &matrix)
            {
                memcpy(data, matrix.data, sizeof(data));
            }

            BaseMatrix4x4(const BaseVector4<TYPE> &euler)
            {
                setEuler(euler);
            }

            BaseMatrix4x4(TYPE x, TYPE y, TYPE z)
            {
                setEuler(x, y, z);
            }

            BaseMatrix4x4(const BaseVector4<TYPE> &axis, TYPE radians)
            {
                setRotation(axis, radians);
            }

            BaseMatrix4x4(const BaseQuaternion<TYPE> &rotation)
            {
                setRotation(rotation);
            }

            BaseMatrix4x4(const BaseQuaternion<TYPE> &rotation, const BaseVector4<TYPE> &translation)
            {
                setRotation(rotation);
                this->translation = translation;
            }

            void setZero(void)
            {
                memset(data, 0, sizeof(data));
            }

            void setIdentity(void)
            {
                _11 = _22 = _33 = _44 = TYPE(1);
                _12 = _13 = _14 = TYPE(0);
                _21 = _23 = _24 = TYPE(0);
                _31 = _32 = _34 = TYPE(0);
                _41 = _42 = _43 = TYPE(0);
            }

            void setScaling(TYPE scalar)
            {
                _11 = scalar;
                _22 = scalar;
                _33 = scalar;
            }

            void setScaling(const BaseVector4<TYPE> &vector)
            {
                _11 = vector.x;
                _22 = vector.y;
                _33 = vector.z;
            }

            void setTranslation(const BaseVector4<TYPE> &nTranslation)
            {
                _41 = nTranslation.x;
                _42 = nTranslation.y;
                _43 = nTranslation.z;
            }

            void setEuler(const BaseVector4<TYPE> &euler)
            {
                setEuler(euler.x, euler.y, euler.z);
            }

            void setEuler(TYPE x, TYPE y, TYPE z)
            {
                TYPE cosX(cos(x));
                TYPE sinX(sin(x));
                TYPE cosY(cos(y));
                TYPE sinY(sin(y));
                TYPE cosZ(cos(z));
                TYPE sinZ(sin(z));
                TYPE cosXsinY(cosX * sinY);
                TYPE sinXsinY(sinX * sinY);

                table[0][0] = ( cosY * cosZ);
                table[1][0] = (-cosY * sinZ);
                table[2][0] =  sinY;
                table[3][0] = TYPE(0);

                table[0][1] = ( sinXsinY * cosZ + cosX * sinZ);
                table[1][1] = (-sinXsinY * sinZ + cosX * cosZ);
                table[2][1] = (-sinX * cosY);
                table[3][1] = TYPE(0);

                table[0][2] = (-cosXsinY * cosZ + sinX * sinZ);
                table[1][2] = ( cosXsinY * sinZ + sinX * cosZ);
                table[2][2] = ( cosX * cosY);
                table[3][2] = TYPE(0);

                table[0][3] = TYPE(0);
                table[1][3] = TYPE(0);
                table[2][3] = TYPE(0);
                table[3][3] = TYPE(1);
            }

            void setRotation(const BaseVector4<TYPE> &axis, TYPE radians)
            {
                TYPE cosAngle(cos(radians));
                TYPE sinAngle(sin(radians));

                table[0][0] = (cosAngle + axis.x * axis.x * (TYPE(1) - cosAngle));
                table[0][1] = ( axis.z * sinAngle + axis.y * axis.x * (TYPE(1) - cosAngle));
                table[0][2] = (-axis.y * sinAngle + axis.z * axis.x * (TYPE(1) - cosAngle));
                table[0][3] = TYPE(0);

                table[1][0] = (-axis.z * sinAngle + axis.x * axis.y * (TYPE(1) - cosAngle));
                table[1][1] = (cosAngle + axis.y * axis.y * (TYPE(1) - cosAngle));
                table[1][2] = ( axis.x * sinAngle + axis.z * axis.y * (TYPE(1) - cosAngle));
                table[1][3] = TYPE(0);

                table[2][0] = ( axis.y * sinAngle + axis.x * axis.z * (TYPE(1) - cosAngle));
                table[2][1] = (-axis.x * sinAngle + axis.y * axis.z * (TYPE(1) - cosAngle));
                table[2][2] = (cosAngle + axis.z * axis.z * (TYPE(1) - cosAngle));
                table[2][3] = TYPE(0);

                table[3][0] = TYPE(0);
                table[3][1] = TYPE(0);
                table[3][2] = TYPE(0);
                table[3][3] = TYPE(1);
            }

            void setRotation(const BaseQuaternion<TYPE> &rotation)
            {
                TYPE xy(rotation.x * rotation.y);
                TYPE zw(rotation.z * rotation.w);
                TYPE xz(rotation.x * rotation.z);
                TYPE yw(rotation.y * rotation.w);
                TYPE yz(rotation.y * rotation.z);
                TYPE xw(rotation.x * rotation.w);
                TYPE squareX(rotation.x * rotation.x);
                TYPE squareY(rotation.y * rotation.y);
                TYPE squareZ(rotation.z * rotation.z);
                TYPE squareW(rotation.w * rotation.w);
                TYPE determinant(TYPE(1) / (squareX + squareY + squareZ + squareW));

                table[0][0] = (( squareX - squareY - squareZ + squareW) * determinant);
                table[0][1] = (TYPE(2) * (xy + zw) * determinant);
                table[0][2] = (TYPE(2) * (xz - yw) * determinant);
                table[0][3] = TYPE(0);

                table[1][0] = (TYPE(2) * (xy - zw) * determinant);
                table[1][1] = ((-squareX + squareY - squareZ + squareW) * determinant);
                table[1][2] = (TYPE(2) * (yz + xw) * determinant);
                table[1][3] = TYPE(0);

                table[2][0] = (TYPE(2) * (xz + yw) * determinant);
                table[2][1] = (TYPE(2) * (yz - xw) * determinant);
                table[2][2] = ((-squareX - squareY + squareZ + squareW) * determinant);
                table[2][3] = TYPE(0);

                table[3][0] = TYPE(0);
                table[3][1] = TYPE(0);
                table[3][2] = TYPE(0);
                table[3][3] = TYPE(1);
            }

            void setRotationX(TYPE radians)
            {
                TYPE cosAngle(cos(radians));
                TYPE sinAngle(sin(radians));
                table[0][0] = TYPE(1); table[0][1] = TYPE(0);   table[0][2] = TYPE(0);  table[0][3] = TYPE(0);
                table[1][0] = TYPE(0); table[1][1] = cosAngle;  table[1][2] = sinAngle; table[1][3] = TYPE(0);
                table[2][0] = TYPE(0); table[2][1] = -sinAngle; table[2][2] = cosAngle; table[2][3] = TYPE(0);
                table[3][0] = TYPE(0); table[3][1] = TYPE(0);   table[3][2] = TYPE(0);  table[3][3] = TYPE(1);
            }

            void setRotationY(TYPE radians)
            {
                TYPE cosAngle(cos(radians));
                TYPE sinAngle(sin(radians));
                table[0][0] = cosAngle; table[0][1] = TYPE(0); table[0][2] = -sinAngle; table[0][3] = TYPE(0);
                table[1][0] = TYPE(0);  table[1][1] = TYPE(1); table[1][2] = TYPE(0);   table[1][3] = TYPE(0);
                table[2][0] = sinAngle; table[2][1] = TYPE(0); table[2][2] = cosAngle;  table[2][3] = TYPE(0);
                table[3][0] = TYPE(0);  table[3][1] = TYPE(0); table[3][2] = TYPE(0);   table[3][3] = TYPE(1);
            }

            void setRotationZ(TYPE radians)
            {
                TYPE cosAngle(cos(radians));
                TYPE sinAngle(sin(radians));
                table[0][0] = cosAngle; table[0][1] = sinAngle; table[0][2] = TYPE(0); table[0][3] = TYPE(0);
                table[1][0] =-sinAngle; table[1][1] = cosAngle; table[1][2] = TYPE(0); table[1][3] = TYPE(0);
                table[2][0] = TYPE(0);  table[2][1] = TYPE(0);  table[2][2] = TYPE(1); table[2][3] = TYPE(0);
                table[3][0] = TYPE(0);  table[3][1] = TYPE(0);  table[3][2] = TYPE(0); table[3][3] = TYPE(1);
            }

            void setOrthographic(TYPE left, TYPE top, TYPE right, TYPE bottom, TYPE nearDepth, TYPE farDepth)
            {
                table[0][0] = (TYPE(2) / (right - left));
                table[1][0] = TYPE(0);
                table[2][0] = TYPE(0);
                table[3][0] = -((right + left) / (right - left));;

                table[0][1] = TYPE(0);
                table[1][1] = (TYPE(2) / (top - bottom));
                table[2][1] = TYPE(0);
                table[3][1] = -((top + bottom) / (top - bottom));

                table[0][2] = TYPE(0);
                table[1][2] = TYPE(0);
                table[2][2] = (-TYPE(2) / (farDepth - nearDepth));
                table[3][2] = -((farDepth + nearDepth) / (farDepth - nearDepth));

                table[0][3] = TYPE(0);
                table[1][3] = TYPE(0);
                table[2][3] = TYPE(0);
                table[3][3] = TYPE(1);
            }

            void setPerspective(TYPE fieldOfView, TYPE aspectRatio, TYPE nearDepth, TYPE farDepth)
            {
                TYPE x(TYPE(1) / tan(fieldOfView * TYPE(0.5)));
                TYPE y(x * aspectRatio);
                TYPE distance(farDepth - nearDepth);

                table[0][0] = x;
                table[0][1] = TYPE(0);
                table[0][2] = TYPE(0);
                table[0][3] = TYPE(0);

                table[1][0] = TYPE(0);
                table[1][1] = y;
                table[1][2] = TYPE(0);
                table[1][3] = TYPE(0);

                table[2][0] = TYPE(0);
                table[2][1] = TYPE(0);
                table[2][2] = ((farDepth + nearDepth) / distance);
                table[2][3] = TYPE(1);

                table[3][0] = TYPE(0);
                table[3][1] = TYPE(0);
                table[3][2] = -((TYPE(2) * farDepth * nearDepth) / distance);
                table[3][3] = TYPE(0);
            }

            void setLookAt(const BaseVector4<TYPE> &source, const BaseVector4<TYPE> &target, const BaseVector4<TYPE> &worldUpVector)
            {
                rz = ((target - source).getNormal());
                rx = (worldUpVector.Cross(rz).getNormal());
                ry = (rz.Cross(rx).getNormal());

                table[0][3] = TYPE(0);
                table[1][3] = TYPE(0);
                table[2][3] = TYPE(0);
                table[3][3] = TYPE(1);

                invert();
            }

            void setLookAt(const BaseVector4<TYPE> &direction, const BaseVector4<TYPE> &worldUpVector)
            {
                rz = (direction.getNormal());
                rx = (worldUpVector.Cross(rz).getNormal());
                ry = (rz.Cross(rx).getNormal());

                table[0][3] = TYPE(0);
                table[1][3] = TYPE(0);
                table[2][3] = TYPE(0);
                table[3][3] = TYPE(1);
            }

            BaseVector3<TYPE> getEuler(void) const
            {
                BaseVector3 euler;
                euler.y = asin(_31);

                TYPE cosAngle = cos(euler.y);
                if (abs(cosAngle) > 0.005)
                {
                    euler.x = atan2(-(_32 / cosAngle), (_33 / cosAngle));
                    euler.z = atan2(-(_21 / cosAngle), (_11 / cosAngle));
                }
                else
                {
                    euler.x = TYPE(0);
                    euler.y = atan2(_12, _22);
                }

                if (euler.x < TYPE(0))
                {
                    euler.x += (Pi * TYPE(2));
                }

                if (euler.y < TYPE(0))
                {
                    euler.y += (Pi * TYPE(2));
                }

                if (euler.z < TYPE(0))
                {
                    euler.z += (Pi * TYPE(2));
                }

                return euler;
            }

            BaseVector3<TYPE> getScaling(void) const
            {
                return BaseVector3(_11, _22, _33);
            }

            TYPE getDeterminant(void) const
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

            BaseMatrix4x4 getTranspose(void) const
            {
                return BaseMatrix4x4({ _11, _21, _31, _41,
                                       _12, _22, _32, _42,
                                       _13, _23, _33, _43, 
                                       _14, _24, _34, _44 });
            }

            BaseMatrix4x4 getInverse(void) const
            {
                TYPE determinant(getDeterminant());
                if (abs(determinant) < _EPSILON)
                {
                    return BaseMatrix4x4();
                }
                else
                {
                    determinant = (TYPE(1) / determinant);

                    BaseMatrix4x4 matrix;
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

            void transpose(void)
            {
                (*this) = getTranspose();
            }

            void invert(void)
            {
                (*this) = getInverse();
            }

            void operator *= (const BaseMatrix4x4 &matrix)
            {
                (*this) = ((*this) * matrix);
            }

            BaseMatrix4x4 operator * (const BaseMatrix4x4 &matrix) const
            {
                BaseMatrix4x4 transpose(matrix.getTranspose());
                return BaseMatrix4x4(rx.Dot(transpose.rx), rx.Dot(transpose.ry), rx.Dot(transpose.rz), rx.Dot(transpose.rw),
                                     ry.Dot(transpose.rx), ry.Dot(transpose.ry), ry.Dot(transpose.rz), ry.Dot(transpose.rw),
                                     rz.Dot(transpose.rx), rz.Dot(transpose.ry), rz.Dot(transpose.rz), rz.Dot(transpose.rw),
                                     rw.Dot(transpose.rx), rw.Dot(transpose.ry), rw.Dot(transpose.rz), rw.Dot(transpose.rw));
            }

            BaseMatrix4x4 operator = (const BaseMatrix4x4 &matrix)
            {
                memcpy(data, matrix.data, sizeof(data));
                return (*this);
            }

            BaseMatrix4x4 operator = (const BaseQuaternion<TYPE> &rotation)
            {
                setRotation(rotation);
                return (*this);
            }

            BaseVector3<TYPE> operator * (const BaseVector3<TYPE> &vector) const
            {
                return BaseVector3(((vector.x * _11) + (vector.y * _21) + (vector.z * _31)),
                                   ((vector.x * _12) + (vector.y * _22) + (vector.z * _32)),
                                   ((vector.x * _13) + (vector.y * _23) + (vector.z * _33)));
            }

            BaseVector4<TYPE> operator * (const BaseVector4<TYPE> &vector) const
            {
                return BaseVector4(((vector.x * _11) + (vector.y * _21) + (vector.z * _31) + (vector.w * _41)),
                                   ((vector.x * _12) + (vector.y * _22) + (vector.z * _32) + (vector.w * _42)),
                                   ((vector.x * _13) + (vector.y * _23) + (vector.z * _33) + (vector.w * _43)),
                                   ((vector.x * _14) + (vector.y * _24) + (vector.z * _34) + (vector.w * _44)));
            }

            BaseMatrix4x4 operator * (TYPE nScalar) const
            {
                return BaseMatrix4x4((_11 * nScalar), (_12 * nScalar), (_13 * nScalar), (_14 * nScalar),
                    (_21 * nScalar), (_22 * nScalar), (_23 * nScalar), (_24 * nScalar),
                    (_31 * nScalar), (_32 * nScalar), (_33 * nScalar), (_34 * nScalar),
                    (_41 * nScalar), (_42 * nScalar), (_43 * nScalar), (_44 * nScalar));
            }

            BaseMatrix4x4 operator + (const BaseMatrix4x4 &matrix) const
            {
                return BaseMatrix4x4(_11 + matrix._11, _12 + matrix._12, _13 + matrix._13, _14 + matrix._14,
                                     _21 + matrix._21, _22 + matrix._22, _23 + matrix._23, _24 + matrix._24,
                                     _31 + matrix._31, _32 + matrix._32, _33 + matrix._33, _34 + matrix._34,
                                     _41 + matrix._41, _42 + matrix._42, _43 + matrix._43, _44 + matrix._44);
            }

            void operator += (const BaseMatrix4x4 &matrix)
            {
                _11 += matrix._11; _12 += matrix._12; _13 += matrix._13; _14 += matrix._14;
                _21 += matrix._21; _22 += matrix._22; _23 += matrix._23; _24 += matrix._24;
                _31 += matrix._31; _32 += matrix._32; _33 += matrix._33; _34 += matrix._34;
                _41 += matrix._41; _42 += matrix._42; _43 += matrix._43; _44 += matrix._44;
            }
        };

        typedef BaseMatrix4x4<float> Float4x4;
    }; // namespace Math
}; // namespace Gek

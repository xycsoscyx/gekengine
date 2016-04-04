#pragma once

#include <xmmintrin.h>
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Math
    {
        struct Quaternion;

        struct Float4x4
        {
        public:
            union
            {
                struct { float data[16]; };
                struct { float table[4][4]; };
                struct { Float4 rows[4]; };
                struct { __m128 simd[4]; };

                struct __declspec(align(16))
                {
                    float _11, _12, _13, _14;
                    float _21, _22, _23, _24;
                    float _31, _32, _33, _34;
                    float _41, _42, _43, _44;
                };

                struct __declspec(align(16))
                {
                    Float4 rx;
                    Float4 ry;
                    Float4 rz;
                    union
                    {
                        struct __declspec(align(16))
                        {
                            Float4 rw;
                        };

                        struct __declspec(align(16))
                        {
                            Float3 translation;
                            float tw;
                        };
                    };
                };

                struct __declspec(align(16))
                {
                    struct { Float3 nx; float nxw; };
                    struct { Float3 ny; float nyw; };
                    struct { Float3 nz; float nzw; };
                    struct __declspec(align(16))
                    {
                        Float3 translation;
                        float tw;
                    };
                };
            };

        public:
            Float4x4(void);
            Float4x4(const __m128(&data)[4]);
            Float4x4(const float(&data)[16]);
            Float4x4(const float *data);
            Float4x4(const Float4x4 &matrix);
            Float4x4(float pitch, float yaw, float roll)
            {
                setEulerRotation(pitch, yaw, roll);
            }

            Float4x4(const Float3 &axis, float radians)
            {
                setAngularRotation(axis, radians);
            }

            void setIdentity(void);
            void setRotationIdentity(void);
            void setScaling(float scalar);
            void setScaling(const Float3 &vector);
            void setEulerRotation(float pitch, float yaw, float roll);
            void setAngularRotation(const Float3 &axis, float radians);
            void setPitchRotation(float radians);
            void setYawRotation(float radians);
            void setRollRotation(float radians);
            void setOrthographic(float left, float top, float right, float bottom, float nearDepth, float farDepth);
            void setPerspective(float fieldOfView, float aspectRatio, float nearDepth, float farDepth);
            void setLookAt(const Float3 &source, const Float3 &target, const Float3 &worldUpVector);
            void setLookAt(const Float3 &direction, const Float3 &worldUpVector);

            Quaternion getQuaternion(void) const;

            Float3 getScaling(void) const;
            float getDeterminant(void) const;
            Float4x4 getTranspose(void) const;
            Float4x4 getInverse(void) const;
            Float4x4 getRotation(void) const;

            void transpose(void);
            void invert(void);

            Float4 operator [] (int row) const;
            Float4 &operator [] (int row);

            operator const float *() const;
            operator float *();

            Float4x4 operator = (const Float4x4 &matrix);

            void operator *= (const Float4x4 &matrix);
            Float4x4 operator * (const Float4x4 &matrix) const;
            Float3 operator * (const Float3 &vector) const;
            Float4 operator * (const Float4 &vector) const;
        };
    }; // namespace Math
}; // namespace Gek

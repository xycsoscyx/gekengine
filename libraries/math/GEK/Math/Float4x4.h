#pragma once

#include <xmmintrin.h>
#include "GEK\Math\Float2.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4.h"

namespace Gek
{
    namespace Math
    {
        struct Quaternion;

        struct Float4x4
        {
        public:
            static const Float4x4 Identity;

        public:
            union
            {
                struct { float data[16]; };
                struct { float table[4][4]; };
                struct { Float4 rows[4]; };
                struct { __m128 simd[4]; };

                struct
                {
                    float _11, _12, _13, _14;
                    float _21, _22, _23, _24;
                    float _31, _32, _33, _34;
                    float _41, _42, _43, _44;
                };

                struct
                {
                    Float4 rx;
                    Float4 ry;
                    Float4 rz;
                    union
                    {
                        struct
                        {
                            Float4 rw;
                        };

                        struct
                        {
                            Float3 translation;
                            float tw;
                        };
                    };
                };

                struct
                {
                    struct { Float3 nx; float nxw; };
                    struct { Float3 ny; float nyw; };
                    struct { Float3 nz; float nzw; };
                    struct
                    {
                        Float3 translation;
                        float tw;
                    };
                };
            };

        public:
            Float4x4(void)
            {
            }

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

			static Math::Float4x4 setScaling(float scale);
			static Math::Float4x4 setScaling(const Float3 &scale);
			static Math::Float4x4 setTranslation(const Float3 &translation);
			static Math::Float4x4 setEulerRotation(float pitch, float yaw, float roll);
			static Math::Float4x4 setAngularRotation(const Float3 &axis, float radians);
			static Math::Float4x4 setPitchRotation(float radians);
			static Math::Float4x4 setYawRotation(float radians);
			static Math::Float4x4 setRollRotation(float radians);
			static Math::Float4x4 setOrthographic(float left, float top, float right, float bottom, float nearClip, float farClip);
			static Math::Float4x4 setPerspective(float fieldOfView, float aspectRatio, float nearClip, float farClip);
			static Math::Float4x4 setLookAt(const Float3 &source, const Float3 &target, const Float3 &worldUpVector);

            Quaternion getQuaternion(void) const;

            Float3 getScaling(void) const;
            float getDeterminant(void) const;
            Float4x4 getTranspose(void) const;
            Float4x4 getInverse(void) const;
            Float4x4 getRotation(void) const;

            Math::Float4x4 &transpose(void);
            Math::Float4x4 &invert(void);

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

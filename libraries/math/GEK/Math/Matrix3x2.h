#pragma once

#include "GEK\Math\Vector2.h"


namespace Gek
{
    namespace Math
    {
        struct Float3x2
        {
        public:
            union
            {
                struct { float data[6]; };
                struct { float table[3][2]; };
                struct { Float2 rows[3]; };

                struct
                {
                    float _11, _12;
                    float _21, _22;
                    float _31, _32;
                };

                struct
                {
                    Float2 rx;
                    Float2 ry;
                    Float2 translation;
                };
            };

        public:
            Float3x2(void)
            {
            }

            Float3x2(const float(&data)[6]);
            Float3x2(const float *data);
            Float3x2(const Float3x2 &matrix);
            Float3x2(float radians)
            {
                setRotation(radians);
            }

            void setIdentity(void);
            void setRotationIdentity(void);
            void setScaling(float scalar);
            void setScaling(const Float2 &vector);
            void setRotation(float radians);

            Float2 getScaling(void) const;

            Float2 operator [] (int row) const;
            Float2 &operator [] (int row);

            operator const float *() const;
            operator float *();

            Float3x2 operator = (const Float3x2 &matrix);

            void operator *= (const Float3x2 &matrix);
            Float3x2 operator * (const Float3x2 &matrix) const;
        };
    }; // namespace Math
}; // namespace Gek

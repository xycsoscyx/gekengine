#pragma once

#include "GEK\Math\Float2.hpp"

namespace Gek
{
    namespace Math
    {
        struct Float3x2
        {
        public:
            static const Float3x2 Identity;

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
            inline Float3x2(void)
            {
            }

            Float3x2(const float(&data)[6]);
            Float3x2(const float *data);
            Float3x2(const Float3x2 &matrix);

            static Float3x2 setScaling(float scale);
            static Float3x2 setScaling(const Float2 &scale);
            static Float3x2 setRotation(float radians);

            Float2 getScaling(void) const;

            Float2 operator [] (int row) const;
            Float2 &operator [] (int row);

            operator const float *() const;
            operator float *();

            Float3x2 &operator = (const Float3x2 &matrix);

            void operator *= (const Float3x2 &matrix);
            Float3x2 operator * (const Float3x2 &matrix) const;
        };
    }; // namespace Math
}; // namespace Gek

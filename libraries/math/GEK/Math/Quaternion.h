#pragma once

#include "GEK\Math\Vector4.h"

namespace Gek
{
    namespace Math
    {
        struct Float3;
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

            Quaternion(const Float4x4 &rotation);
            Quaternion(const Float3 &axis, float radians);
            Quaternion(const Float3 &euler);
            Quaternion(float x, float y, float z);

            void setIdentity(void);
            void setEuler(const Float3 &euler);
            void setEuler(float x, float y, float z);
            void setRotation(const Float3 &axis, float radians);
            void setRotation(const Float4x4 &rotation);

            Float3 getEuler(void) const;
            Quaternion getInverse(void) const;

            void invert(void);
            Quaternion slerp(const Quaternion &rotation, float factor) const;

            Float3 operator * (const Float3 &vector) const;
            Quaternion operator * (const Quaternion &rotation) const;
            void operator *= (const Quaternion &rotation);
            Quaternion operator = (const Float4 &vector);
            Quaternion operator = (const Quaternion &rotation);
            Quaternion operator = (const Float4x4 &rotation);
        };
    }; // namespace Math
}; // namespace Gek

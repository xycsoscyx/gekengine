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
            Quaternion(void);
            Quaternion(__m128 data);
            Quaternion(const float(&data)[4]);
            Quaternion(const float *data);
            Quaternion(const Float4 &vector);
            Quaternion(float x, float y, float z, float w);
            Quaternion(const Float4x4 &rotation);
            Quaternion(const Float3 &axis, float radians);
            Quaternion(const Float3 &euler);
            Quaternion(float x, float y, float z);

            void setIdentity(void);
            void setLength(float length);
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

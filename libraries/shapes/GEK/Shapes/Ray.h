#pragma once

#include "GEK\Math\Vector3.h"

namespace Gek
{
    namespace Shapes
    {
        struct OrientedBox;

        struct Ray
        {
        public:
            Math::Float3 origin;
            Math::Float3 normal;

        public:
            Ray(void);
            Ray(const Math::Float3 &origin, const Math::Float3 &normal);
            Ray(const Ray &ray);

            Ray operator = (const Ray &ray);

            float getDistance(const OrientedBox &orientedBox) const;
        };
    }; // namespace Shapes
}; // namespace Gek

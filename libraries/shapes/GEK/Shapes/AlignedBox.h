#pragma once

#include "GEK\Math\Float3.h"

namespace Gek
{
    namespace Shapes
    {
        struct Plane;

        struct AlignedBox
        {
        public:
            Math::Float3 minimum;
            Math::Float3 maximum;

        public:
            AlignedBox(void);
            AlignedBox(const AlignedBox &box);
            AlignedBox(float size);
            AlignedBox(const Math::Float3 &minimum, const Math::Float3 &maximum);

            AlignedBox operator = (const AlignedBox &box);

            void extend(Math::Float3 &point);

            Math::Float3 getSize(void) const;
            Math::Float3 getCenter(void) const;
            int getPosition(const Plane &plane) const;
        };
    }; // namespace Shapes
}; // namespace Gek

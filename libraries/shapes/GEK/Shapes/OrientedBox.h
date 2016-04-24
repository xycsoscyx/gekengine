#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Shapes\AlignedBox.h"

namespace Gek
{
    namespace Shapes
    {
        struct Plane;

        struct OrientedBox
        {
        public:
            Math::Float4x4 matrix;
            Math::Float3 halfsize;

        public:
            OrientedBox(void);
            OrientedBox(const OrientedBox &box);
            OrientedBox(const AlignedBox &box, const Math::Quaternion &rotation, const Math::Float3 &translation);
            OrientedBox(const AlignedBox &box, const Math::Float4x4 &matrix);

            OrientedBox operator = (const OrientedBox &box);

            int getPosition(const Plane &plane) const;
        };
    }; // namespace Shapes
}; // namespace Gek

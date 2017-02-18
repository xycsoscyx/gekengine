/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/Plane.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Utility/Allocator.hpp"
#include <vector>

namespace Gek
{
    namespace Shapes
    {
        struct Frustum
        {
        public:
            Plane planeList[6];

        public:
            Frustum(void);
            Frustum(const Frustum &frustum);
            Frustum(Math::Float4x4 const &perspectiveTransform);

            void create(Math::Float4x4 const &perspectiveTransform);

            void cull(const std::vector<Sphere, AlignedAllocator<Sphere, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList);
            void cull(const std::vector<AlignedBox, AlignedAllocator<AlignedBox, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList);
            void cull(const std::vector<OrientedBox, AlignedAllocator<OrientedBox, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList);
        };
    }; // namespace Shapes
}; // namespace Gek

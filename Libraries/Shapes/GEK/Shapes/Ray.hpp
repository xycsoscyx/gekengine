/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector3.hpp"

namespace Gek
{
    namespace Shapes
    {
        struct Ray
        {
        public:
            Math::Float3 origin;
            Math::Float3 normal;

        public:
            Ray(void) noexcept;
            Ray(Math::Float3 const &origin, Math::Float3 const &normal) noexcept;
            Ray(const Ray &ray) noexcept;

            Ray &operator = (const Ray &ray) noexcept;
        };
    }; // namespace Shapes
}; // namespace Gek
